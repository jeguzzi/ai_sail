#include "logging.h"
#include "camera.h"
#include "io.h"
#include "led.h"
#include "nina.h"
#include "bsp/camera.h"

#define MAX_CAMERA_SIZE (324 * 324)

static struct pi_device camera;
image_acquisition_t image_acquisition = {.device=&camera, .initialized=0};
static pi_task_t image_task;

static PI_L2 unsigned char *camera_buffer;
static int camera_buffer_size;
static camera_config_t desired_camera_config;
static int _should_set_camera_config;
static stream_config_t desired_stream_config;
static int _should_set_stream_config;
static int number_of_frames = 0;
image_streaming_t image_streaming = {};

unsigned char * crop(unsigned char * buffer) {
  image_acquisition_t * ia = &image_acquisition;
  if(ia->should_crop) {
    const unsigned char * source = buffer + ia->camera_width * ia->config.top + ia->config.left;
    unsigned char * dest = buffer;
    int step = ia->config.step;
    if(step == 1) {
      if(!ia->config.left && !ia->config.right) {
        return (unsigned char *) source;
      }
      for(int i=0; i<ia->cropped_height; i++) {
        memcpy(dest, source, ia->cropped_width);
        dest += ia->cropped_width;
        source += ia->camera_width;
      }
      return buffer;
    }
    const unsigned char * o;
    for(int i=0; i<ia->cropped_height; i+=step) {
      o = source;
      for(int j=0; j<ia->cropped_width; j+=step, o+=step) {
        *dest++ = *o;
      }
      source += step * ia->camera_width;
    }
    return buffer;
  }
}

static void update_streamer() {
  if(!image_streaming.streamer) return;
  image_acquisition_t *ia = &image_acquisition;
  pi_buffer_set_format(&(image_streaming.buffer), ia->width, ia->height, 1, PI_BUFFER_FORMAT_GRAY);
  int color = 0; // Grazyscale
  if((ia->config.format == FULL || ia->config.format == QVGA) && ia->config.step == 1) {
    // BAYER
    color = (ia->config.top % 2 << 1) + (ia->config.left % 2) + 1;
  }
  set_streamer(image_streaming.streamer, ia->width, ia->height, color);
}

void _set_stream_config(stream_config_t *config) {
  if(image_streaming.config.on && !config->on) {
    image_streaming.config.on = 0;
    // put_nina_to_sleep();
  }
  else if(!image_streaming.config.on && config->on) {
    if(!image_streaming.streamer) {
      struct pi_device *transport = NULL;
#ifdef TRANSPORT_WIFI
      if(config->transport == TRANSPORT_WIFI) {
        // wake_up_nina();
        transport = open_wifi();
      }
#endif
#ifdef TRANSPORT_PIPE
      if(config->transport == TRANSPORT_PIPE) {
        transport = open_pipe("/tmp/image_pipe");
      }
#endif
      if(transport) {
        frame_streamer_t *streamer = init_streamer(
            transport, config->format, image_acquisition.width, image_acquisition.height);
        if(streamer) {
          image_streaming.streamer = streamer;
          update_streamer();
          image_streaming.config.on = 1;
        }
      }
    }
    else {
      image_streaming.config.on = 1;
    }
  }
  // Not allowed to change format of an initialized streamer
  if(!image_streaming.streamer) {
    if(config->format < 2)
      image_streaming.config.format = config->format;
    if(config->transport < 2)
      image_streaming.config.transport = config->transport;
  }
#ifdef UART_COMM
  send_stream_config(&(image_streaming.config));
#endif
  LOG("Has set stream to on = %d, format = %d, transport = %d\n",
      image_streaming.config.on, image_streaming.config.format, image_streaming.config.transport);
}

void set_stream_config(stream_config_t *_config) {
  desired_stream_config = *_config;
  _should_set_stream_config = 1;
}

static int set_size(camera_config_t c) {
  int camera_width, camera_height;
  switch(c.format){
    case QVGA:
      camera_width = 324;
      camera_height = 244;
      break;
    case QQVGA:
      camera_width = 162;
      camera_height = 122;
      break;
    case HALF:
      camera_width = 162;
      camera_height = 162;
      break;
    case FULL:
      camera_width = 324;
      camera_height = 324;
      break;
    default:
      LOG_ERROR("Unknown format %d\n", c.format);
      return 1;
  }
  if(c.top < 0 || c.bottom <0 || (c.top + c.bottom) >  camera_height) {
    LOG_ERROR("Wrong image size top %d, bottom %d, height = %d\n",
        c.top, c.bottom, camera_height);
    return 1;
  }
  if(c.left < 0 || c.right <0 || (c.left + c.right) >  camera_width) {
    LOG_ERROR("Wrong image size left %d, right %d, width = %d\n",
        c.left, c.right, camera_width);
    return 1;
  }
  if(c.step <= 0 || c.step >= camera_width || c.step >= camera_height) {
    LOG_ERROR("Wrong step size %d, width = %d, height = %d\n",
        c.step, camera_width, camera_height);
    return 1;
  }
  image_acquisition_t * ia = &image_acquisition;

/* Not allowed to change width and height if
  - DYNAMIC_IMAGE_SIZE is not defined
  - or a JPEG streamer is already configured */
  int width = (camera_width - c.left - c.right)/ c.step;
  int height = (camera_height - c.bottom - c.top) / c.step;
  int camera_size = camera_width * camera_height;
#ifndef DYNAMIC_IMAGE_SIZE
  camera_size -= camera_width * c.bottom;
#endif
  if(camera_size > camera_buffer_size && ia->initialized) {
    // TODO (Jerome): could realloc, for now do not allow to enlarge the buffer
    LOG_ERROR("Camera size %d is larger than the allocated buffer size %d\n",
              camera_size, camera_buffer_size);
    return -1;
  }
  if((image_streaming.streamer && image_streaming.config.format == FRAME_STREAMER_FORMAT_JPEG)
#ifndef DYNAMIC_IMAGE_SIZE
    || ia->initialized
#endif
  ) {
    if(width != ia->width || height != ia->height) {
      LOG_ERROR("Not allowed to change image size from %d x %d\n", ia->width, ia->height);
      return -1;
    }
  }
  ia->camera_height = camera_height;
  ia->camera_width = camera_width;
  ia->cropped_width = ia->camera_width - c.left - c.right;
  ia->cropped_height = ia->camera_height - c.bottom - c.top;
  ia->should_crop = c.left || c.right || c.bottom || c.top || (c.step > 1);
  ia->width = width;
  ia->height = height;
  ia->config.top = c.top;
  ia->config.bottom = c.bottom;
  ia->config.left = c.left;
  ia->config.right = c.right;
  ia->config.step = c.step;
  ia->camera_size = ia->camera_width * ia->camera_height * sizeof(unsigned char);
  ia->size = ia->width * ia->height * sizeof(unsigned char);
  LOG("Will capture %d x %d frames, crop (%d) with margins of %d %d %d %d pixels, "
      "sample every %d pixels, to finally get %d x %d frames\n",
       ia->camera_width, ia->camera_height, ia->should_crop,
       ia->config.top, ia->config.right, ia->config.bottom, ia->config.left,
       ia->config.step, ia->width, ia->height);
  return 0;
}

void set_camera_config(camera_config_t *_config) {
  desired_camera_config = *_config;
  _should_set_camera_config = 1;
}

int _set_camera_config(camera_config_t *_config) {
  LOG("Should set camera config to margin = [%d %d %d %d], step = %d, format = %d, target = %d, ae = %d, fps = %d\n",
      _config->top, _config->right, _config->bottom, _config->left,
      _config->step, _config->format, _config->target_value, _config->ae, _config->fps);
  int error = 0;
  image_acquisition_t * ia = &image_acquisition;
  error = set_size(*_config);
  if(error) goto done;
  if(ia->config.format != _config->format &&  _config->format >=0 && _config->format < 4
#ifndef DYNAMIC_CAMERA_SIZE
    && !ia->initialized
#endif
    ) {
    himax_set_format(&camera, _config->format);
    ia->config.format = _config->format;
  }
  if(ia->config.ae != _config->ae) {
    himax_enable_ae(&camera, _config->ae);
    ia->config.ae = _config->ae;
  }
  if(ia->config.target_value != _config->target_value) {
    himax_set_target_value(&camera, _config->target_value);
    ia->config.target_value = _config->target_value;
  }
  if(ia->config.fps != _config->fps) {
    ia->config.fps = himax_set_fps(&camera, _config->fps, ia->config.format);
  }
  update_streamer();
  done:
#ifdef UART_COMM
  send_camera_config(&(ia->config));
#endif
  LOG("Has set camera config to margin = [%d %d %d %d], step = %d, format = %d, target = %d, ae = %d, fps = %d\n",
      ia->config.top, ia->config.right, ia->config.bottom, ia->config.left,
      ia->config.step, ia->config.format, ia->config.target_value, ia->config.ae, ia->config.fps);
#ifdef UART_COMM
  if(error)
    uart_flush_rx();
#endif
  return error;
}

// with respect to streaming
static int finish_streaming_before_external_event;
static int should_wait_for_external;
static int waiting;
static pi_task_t stream_task;
static pi_task_t external_task;

enum {external = 1, stream = 2};

static image_event_t image_event;

image_event_t wait_for_new_image() {
  while(!image_event.image) pi_yield();
  image_event_t e = image_event;
  image_event = (image_event_t){};
  return e;
}

static void enqueue_capture(void * arg);

void start_camera_loop(bool wait_external, bool concurrent) {
  finish_streaming_before_external_event = !concurrent;
  should_wait_for_external = wait_external;
  // allocate the memory of L2 for the image buffer
#ifdef DYNAMIC_CAMERA_SIZE
  LOG("Allocating a large enough buffer to allow for any change of camera settings.\n")
  camera_buffer_size = MAX_CAMERA_SIZE;
#else
  LOG("Allocating a minimal large buffer for the current camera settings.\n")
  camera_buffer_size = image_acquisition.camera_size - image_acquisition.config.bottom * image_acquisition.camera_width;
#endif
  camera_buffer = pi_l2_malloc(camera_buffer_size);
  if (camera_buffer == NULL){
    LOG_ERROR("Failed to alloc L2 memeory of size %d\n", camera_buffer_size);
    return;
  }
  image_acquisition.initialized = 1;
  number_of_frames = 0;
  pi_buffer_init(&(image_streaming.buffer), PI_BUFFER_TYPE_L2, camera_buffer);
  waiting = 0;
  enqueue_capture(NULL);
}

static void external_event_async(void * arg) {
  unsigned char * image = (unsigned char *) arg;
  if(should_wait_for_external) {
    waiting |= external;
    image_event = (image_event_t){.image=image, .release_task=pi_task_callback(&external_task, enqueue_capture, &external_task)};
  }
  else{
    image_event = (image_event_t){.image=image, .release_task=NULL};
  }
}

static void end_of_frame(void *arg) {
  number_of_frames++;
  pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
  if(_should_set_camera_config) {
    _set_camera_config(&desired_camera_config);
    _should_set_camera_config = 0;
  }
  if(_should_set_stream_config) {
    _set_stream_config(&desired_stream_config);
    _should_set_stream_config = 0;
  }
  // LOG("New frame %03d %03d %03d %03d ...\n", camera_buffer[0], camera_buffer[1], camera_buffer[2], camera_buffer[3]);
  if(!(number_of_frames % LED_PERIOD))
    toggle_led();
  if(!image_acquisition.config.ae)
    himax_update_exposure(&camera);
  unsigned char * image = crop(camera_buffer);
  if(image_streaming.config.on && image_streaming.streamer) {
    image_streaming.buffer.data = image;
    pi_task_t *t;
    if(finish_streaming_before_external_event && should_wait_for_external) {
      t = pi_task_callback(&stream_task, external_event_async, image);
    }
    else {
      waiting |= stream;
      t = pi_task_callback(&stream_task, enqueue_capture, &stream_task);
      external_event_async(image);
    }
    frame_streamer_send_async(image_streaming.streamer, &(image_streaming.buffer), t);
    return;
  }
  external_event_async(image);
  if(!should_wait_for_external)
  {
    enqueue_capture(NULL);
  }
}

static void enqueue_capture(void * arg) {
  if(arg == &stream_task) {
    waiting &= ~stream;
    // printf("-S -> %d\n", waiting);
  }
  if(arg == &external_task) {
    waiting &= ~external;
    // printf("-E -> %d\n", waiting);
  }
  if(waiting) return;
  pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
  // Do not acquire pixels below the bottom margin, so we avoid grabber bug (and speed up)
  int size = image_acquisition.camera_size - image_acquisition.config.bottom * image_acquisition.camera_width;
  pi_camera_capture_async(&camera, camera_buffer, size, pi_task_callback(&image_task, end_of_frame, NULL));
}

int init_camera(camera_config_t _camera_config, stream_config_t _stream_config) {
  if(himax_open(&camera))
    return -1;
  himax_configure(&camera, FIX_EXPOSURE_AFTER);
  if(_set_camera_config(&_camera_config))
    return -1;

  #ifndef GVSOC
  pi_time_wait_us(1000000);
  #endif

  _set_stream_config(&_stream_config);
  LOG("Initialized Himax camera\n");
  return 0;
}

void close_camera() {
  //TODO (GAPSDK): should allow to free streamer
  himax_close(&camera);
  if(camera_buffer_size && camera_buffer) {
    pi_l2_free(camera_buffer, camera_buffer_size);
  }
  LOG("Closed camera\n");
}
