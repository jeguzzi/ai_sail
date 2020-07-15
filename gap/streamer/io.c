#include <stdio.h>
#include "io.h"
#include "logging.h"
#include "bsp/fs.h"
#include "bsp/transport.h"
#include "bsp/bsp.h"

#define CHUNK_SIZE 8192
static uint8_t jpeg_header[306];
static uint8_t jpeg_footer[2];
typedef enum
{
  HEADER, /* Next packet is header */
  FOOTER, /* Next packet is footer */
  DATA /* Next packet is data */
} ImageState_t;
typedef enum
{
  RAW,
  JPEG,
  UNKNOWN
} image_format_t;
static ImageState_t state = HEADER;
static image_format_t format = UNKNOWN;
#define RAW_HEADER "?IMG"
uint16_t raw_header[5];
const char raw_footer[] = "!IMG";
static bool new_frame = 0;
typedef struct pi_transport_header header_t;

static int send_fifo(void *fifo, const char*buffer, size_t size)
{
  int ret = 0;
  int steps = size / CHUNK_SIZE;
  for(int i=0; i<steps; i++){
    ret += pi_fs_write(fifo, (void*) (buffer + CHUNK_SIZE*i), CHUNK_SIZE);
  }
  if((size % CHUNK_SIZE) != 0)
    ret += pi_fs_write(fifo, (void*) (buffer + CHUNK_SIZE*steps), (size % CHUNK_SIZE) * sizeof(unsigned char));
  return ret;
}

static void handle_image_stream(void *fifo, uint8_t *buffer, int32_t size) {
  int32_t *buffer32 = (int32_t *)buffer;
  header_t *header;
  switch (buffer32[0])
  {
    case 1:  // INITIAL REQUEST
      if(size > 95)
      {
        format = (image_format_t) buffer32[3];
        state = HEADER;
        // printf("Got streaming request for %s with format %d == RAW, %d == JPEG\n",
        //        buffer+32, format == RAW, format == JPEG);
      }
      break;
    case 1024: // HEADER
        header = (header_t *) buffer;
        // printf("Header %d %d %d\n", header->channel, header->packet_size, header->info);
        if(header->info <= 1 && format == JPEG)
        {
          new_frame = (header->info == 1);
          // printf("new_frame: %d\n", new_frame);
        }
        else if (header->info > 1 && format == RAW){
          // RAW
          raw_header[2] = (int16_t) (0xFFFF & header->info);
          raw_header[3] = (int16_t) (header->packet_size / raw_header[2]);
          raw_header[4] = (int16_t) (header->info >> 16);
          // printf("Received first RAW HEADER: shape (%d, %d), color %d\n",
          //        raw_header[2], raw_header[3], raw_header[4]);
          //printf("RAW HEADER 10 %04x  %04x %04x...\n", raw_header[0], raw_header[1], raw_header[2]);
          send_fifo(fifo, (char *)raw_header, 10);
        }
        else{
          LOG_ERROR("Unexpected header %d %d %d\n", header->channel, header->packet_size, header->info);
        }
        break;
      default:
        // printf("Should read image %d \n", size);
        if(format == RAW)
        {
          send_fifo(fifo, (char *)buffer, size);
          send_fifo(fifo, raw_footer, 4);
        }
        else if(format == JPEG)
        {
          switch (state) {
            case HEADER:
              // printf("Setting JPEG header of %i bytes", size);
              memcpy(&jpeg_header, (uint8_t*) buffer, sizeof(jpeg_header));
              send_fifo(fifo, (const char*) &jpeg_header, sizeof(jpeg_header) );
              state = FOOTER;
              break;
            case FOOTER:
              // printf("Setting JPEG footer of %i bytes", size);
              memcpy(&jpeg_footer, (uint8_t*) buffer, sizeof(jpeg_footer));
              state = DATA;
              break;
            case DATA:
              send_fifo(fifo, (const char*) buffer, size);
              if (new_frame) {
                send_fifo(fifo, (const char*) &jpeg_footer, sizeof(jpeg_footer) );
                send_fifo(fifo, (const char*) &jpeg_header, sizeof(jpeg_header) );
              }
              break;
          }
        }
        break;
  }
}

int __pipe_open(struct pi_device *device)
{
  if(device->data) return 0;
  return -1;
}

int __pipe_connect(struct pi_device *device, int channel, void (*rcv_callback(void *arg, struct pi_transport_header)), void *arg)
{
  return 0;
}

int __pipe_send_async(struct pi_device *device, void *buffer, size_t size, pi_task_t *task)
{
  // printf("pipe api send async\n");
  void *fifo = device->data;
  if(!fifo) return -1;
  // at first a sync implementation to check the interface!
  handle_image_stream(fifo, (uint8_t *)buffer, (int32_t) size);
  // printf("-- pipe api send async\n");
  pi_task_push(task);
  return 0;
}

void __pipe_close(struct pi_device *device)
{
  // printf("pipe api close\n");
}

int __pipe_receive_async(struct pi_device *device, void *buffer, size_t size, pi_task_t *task)
{
  // printf("pipe api receive async\n");
  return 0;
}


static pi_transport_api_t pipe_api =
{
  .open              = &__pipe_open,
  .connect           = &__pipe_connect,
  .send_async        = &__pipe_send_async,
  .receive_async     = &__pipe_receive_async,
  .close             = &__pipe_close,
};

static struct pi_transport_conf pipe_transport_conf = {.api = &pipe_api};
static struct pi_device fs;
static struct pi_device pipe = {.data=NULL, .config=&pipe_transport_conf};

static int init_fs()
{
  struct pi_fs_conf conf;
  pi_fs_conf_init(&conf);
  conf.type = PI_FS_HOST;
  pi_open_from_conf(&fs, &conf);
  return pi_fs_mount(&fs);
}

static void close_fs()
{
  pi_fs_unmount(&fs);
}

struct pi_device * open_pipe(char *path)
{
  if(pipe.data) return NULL;
  init_fs();
  void *fifo = pi_fs_open(&fs, path, PI_FS_FLAGS_WRITE);
  if(!fifo)
  {
    LOG_ERROR("Failed to open pipe\n");
    return NULL;
  }
  pipe.data = fifo;
  pi_transport_open(&pipe);
  memcpy(raw_header, RAW_HEADER, 4);
  LOG("Opened pipe\n");
  return &pipe;
}

void close_pipe()
{
  if(pipe.data) return;
  pi_fs_close(pipe.data);
  close_fs();
  LOG("Closed pipe\n");
}
