#include "stdio.h"

#include "pmsis.h"
#include "bsp/camera.h"

#include "frame_streamer_utils.h"

#include "camera.h"
#include "nina.h"
#include "common.h"

#define SEQ 0
#define PIPE 1
#define PIPE2 2
#define PIPE3 3
#define PIPE2W 4
#define PIPEE 5
#define PIPE2E 6

#define FORMAT  HALF
#define LOOP SEQ
#define FPS 30
#define PERF
// #define PRINT_STATS
#define NUMBER 100
#define IGNORE_BOTTOM
// #define IGNORE_LAST 0
#define SEND
// #define JUST_STREAM_AFTER 10

extern image_acquisition_t image_acquisition;
extern image_streaming_t image_streaming;
extern unsigned char * crop(unsigned char *);


static camera_config_t camera_config = {
  .top = 33,
  .right = 2,
  .bottom = 33,
  .left = 0,
  .format = HALF,
  .step = 1,
  .target_value = 0x70,
  .fps = FPS,
};

static stream_config_t stream_config = {
#ifdef SEND
  .on = 1,
#else
  .on = 0,
#endif
  .format = 0,
  .transport = TRANSPORT_WIFI
};

static void test_camera_loop();
static void print_stats();
static int frames_acquired;
static int frames_sent;
static struct pi_device *camera;

#if LOOP == SEQ
#define LOOP_NAME "SEQ"
// WORKS WELL
// CAVEAT:
//  - If I disable AE (without calibration) it resets the camera
//    or does something else very strange
// If we ignore at least the last line, we get almost full FPS, else 1/2 FPS.

static int size;
static pi_task_t task1;
static pi_task_t task2;
static PI_L2 unsigned char *camera_buffer;
static void frame_sent(void *task);
static void frame_acquired(void *task);

#ifdef PERF
static int acquired[NUMBER];
static int sent[NUMBER];

void print_stats()
{
  printf("acquired=[");
  for (size_t i = 0; i < NUMBER; i++) {
    printf("%d,", acquired[i]);
  }
  printf("]\n");

  printf("sent=[");
  for (size_t i = 0; i < NUMBER; i++) {
    printf("%d,", sent[i]);
  }
  printf("]\n");
}
#endif

static void just_stream(void *task);
static void frame_acquired(void * task);
static void frame_sent(void * task);

void test_camera_loop()
{
  size = image_acquisition.camera_size;
#ifdef IGNORE_BOTTOM
  size -= image_acquisition.config.bottom * image_acquisition.camera_width;
#elif IGNORE_LAST
  size -= IGNORE_LAST;
#endif
  camera_buffer = pi_l2_malloc(size);
  if (camera_buffer == NULL){
    LOG_ERROR("Failed to alloc L2 memory of size %d\n", size);
    return;
  }
  pi_buffer_init(&(image_streaming.buffer), PI_BUFFER_TYPE_L2, camera_buffer);
  pi_camera_control(camera, PI_CAMERA_CMD_START, 0);
  pi_camera_capture_async(camera, camera_buffer, size, pi_task_callback(&task1, frame_acquired, NULL));
}

#ifdef JUST_STREAM_AFTER

static void just_stream(void *task) {
  frames_sent++;
#ifdef PERF
  sent[frames_sent] = pi_perf_read(PI_PERF_CYCLES);
#endif
#ifdef SEND
  frame_streamer_send_async(streamer, &buffer, pi_task_callback(&task1, just_stream, NULL));
#else
  just_stream(NULL);
#endif
}

static void frame_acquired(void *task) {
  pi_camera_control(camera, PI_CAMERA_CMD_STOP, 0);
#ifdef PERF
  acquired[frames_acquired] = pi_perf_read(PI_PERF_CYCLES);
#endif
  buffer.data = crop(camera_buffer);
  frames_acquired++;
#ifdef SEND
  if(frames_acquired > JUST_STREAM_AFTER)
    frame_streamer_send_async(image_streaming.streamer, &(image_streaming.buffer), pi_task_callback(&task1, just_stream, NULL));
  else
    frame_streamer_send_async(image_streaming.streamer, &(image_streaming.buffer), pi_task_callback(&task1, frame_sent, NULL));
#else
  frame_sent(NULL);
#endif
}

#else

static void frame_acquired(void *task) {
  pi_camera_control(camera, PI_CAMERA_CMD_STOP, 0);
#ifdef PERF
  acquired[frames_acquired] = pi_perf_read(PI_PERF_CYCLES);
#endif
  buffer.data = crop(camera_buffer);
  frames_acquired++;
#ifdef SEND
  frame_streamer_send_async(image_streaming.streamer, &(image_streaming.buffer), pi_task_callback(&task1, frame_sent, NULL));
#else
  frame_sent(NULL);
#endif
}
#endif

static void frame_sent(void * task) {
  #ifdef PERF
    sent[frames_sent] = pi_perf_read(PI_PERF_CYCLES);
    frames_sent++;
  #endif
  pi_camera_control(camera, PI_CAMERA_CMD_START, 0);
  pi_camera_capture_async(camera, camera_buffer, size, pi_task_callback(&task1, frame_acquired, NULL));
}

#endif

int main()
{
  pi_freq_set(PI_FREQ_DOMAIN_FC, 100000000);
  pi_time_wait_us(100000);
#ifdef SEND
  init_nina(1);
  pi_time_wait_us(1000000);
#endif
  if(init_camera(camera_config, stream_config))
  {
    pmsis_exit(-1);
  }
  camera = image_acquisition.device;
  printf("\n-------- Starting loop %s --------\n", LOOP_NAME);
#ifdef PERF
  pi_perf_conf(1<<PI_PERF_CYCLES);
  pi_perf_start();
  pi_perf_reset();
  int start = pi_perf_read(PI_PERF_CYCLES);
#endif
  test_camera_loop();
  while(NUMBER < 0 || (frames_acquired < NUMBER && frames_sent < NUMBER)) pi_yield();
  printf("\n-------- Ended loop --------\n");
  printf("acquired %d and sent %d frames", frames_acquired, frames_sent);
#ifdef PERF
  int end = pi_perf_read(PI_PERF_CYCLES);
  int dt = (end - start) / 100000;
  printf(" in %d ms (%.1f/%.1f fps)\n", dt, 1000.0 * frames_acquired / dt, 1000.0 * frames_sent / dt);
#ifdef PRINT_STATS
  print_stats();
#endif
#endif
  printf("\n");
  close_camera();
#ifdef SEND
  close_nina();
#endif
  return 0;
}
