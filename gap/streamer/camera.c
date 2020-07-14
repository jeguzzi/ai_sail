#include "common.h"
#include "camera.h"
#include "io.h"
#include "led.h"
#include "nina.h"
#include "bsp/camera.h"

#define MAX_CAMERA_SIZE (324 * 324)

// TODO: add a flag for dynamic / static (static won't allow to change size (?just step?)

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

// TODO: (Jerome) Check this new version

unsigned char * crop(unsigned char * buffer)
{
  image_acquisition_t * ia = &image_acquisition;
  if(ia->should_crop)
  {
    const unsigned char * source = buffer + ia->camera_width * ia->config.top + ia->config.left;
    unsigned char * dest = buffer;
    int step = ia->config.step;
    if(step == 1)
    {
      if(!ia->config.left && !ia->config.right)
      {
        return (unsigned char *) source;
      }
      for(int i=0; i<ia->cropped_height; i++)
      {
        memcpy(dest, source, ia->cropped_width);
        dest += ia->cropped_width;
        source += ia->camera_width;
      }
      return buffer;
    }
    const unsigned char * o;
    for(int i=0; i<ia->cropped_height; i+=step)
    {
      o = source;
      for(int j=0; j<ia->cropped_width; j+=step, o+=step)
      {
        *dest++ = *o;
      }
      source += step * ia->camera_width;
    }
    return buffer;
  }
}

static void update_streamer()
{
  if(!image_streaming.streamer) return;
  image_acquisition_t *ia = &image_acquisition;
  pi_buffer_set_format(&(image_streaming.buffer), ia->width, ia->height, 1, PI_BUFFER_FORMAT_GRAY);
  int color = 0; // Grazyscale
  if((ia->config.format == FULL || ia->config.format == QVGA) && ia->config.step == 1)
  {
    // BAYER
    color = (ia->config.top % 2 << 1) + (ia->config.left % 2) + 1;
  }
  set_streamer(image_streaming.streamer, ia->width, ia->height, color);
}

void _set_stream_config(stream_config_t *config)
{
  if(image_streaming.config.on && !config->on)
  {
    image_streaming.config.on = 0;
    // put_nina_to_sleep();
  }
  else if(!image_streaming.config.on && config->on)
  {
    if(!image_streaming.streamer)
    {
      struct pi_device *transport = NULL;
#ifdef TRANSPORT_WIFI
      if(config->transport == TRANSPORT_WIFI)
      {
        // wake_up_nina();
        transport = open_wifi();
      }
#endif
#ifdef TRANSPORT_PIPE
      if(config->transport == TRANSPORT_PIPE)
      {
        transport = open_pipe("/tmp/image_pipe");
      }
#endif
      if(transport)
      {
        frame_streamer_t *streamer = init_streamer(
            transport, config->format, image_acquisition.width, image_acquisition.height);
        if(streamer)
        {
          image_streaming.streamer = streamer;
          update_streamer();
          image_streaming.config.on = 1;
        }
      }
    }
    else{
      image_streaming.config.on = 1;
    }
  }
  // Not allowed to change format of an initialized streamer
  if(!image_streaming.streamer)
  {
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

void set_stream_config(stream_config_t *_config)
{
  desired_stream_config = *_config;
  _should_set_stream_config = 1;
}

static int set_size(camera_config_t c)
{
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
  if(c.top < 0 || c.bottom <0 || (c.top + c.bottom) >  camera_height)
  {
    LOG_ERROR("Wrong image size top %d, bottom %d, height = %d\n",
        c.top, c.bottom, camera_height);
    return 1;
  }
  if(c.left < 0 || c.right <0 || (c.left + c.right) >  camera_width)
  {
    LOG_ERROR("Wrong image size left %d, right %d, width = %d\n",
        c.left, c.right, camera_width);
    return 1;
  }
  if(c.step <= 0 || c.step >= camera_width || c.step >= camera_height)
  {
    LOG_ERROR("Wrong step size %d, width = %d, height = %d\n",
        c.step, camera_width, camera_height);
    return 1;
  }
  image_acquisition_t * ia = &image_acquisition;

// Not allowed to change width and height if
// - DYNAMIC_IMAGE_SIZE is not defined
// - or a JPEG streamer is already configured
  int width = (camera_width - c.left - c.right)/ c.step;
  int height = (camera_height - c.bottom - c.top) / c.step;
  int camera_size = camera_width * camera_height;
  if(camera_size > camera_buffer_size && ia->initialized)
  {
    // Could realloc, for now do not allow to enlarge the buffer
    LOG_ERROR("Camera size %d is larger than the allocated buffer size %d\n",
              camera_size, camera_buffer_size);
    return -1;
  }
  if((image_streaming.streamer && image_streaming.config.format == FRAME_STREAMER_FORMAT_JPEG)
#ifndef DYNAMIC_IMAGE_SIZE
    || ia->initialized
#endif
  )
  {
    if(width != ia->width || height != ia->height){
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
  // size = CAM_DSMPL_W * CAM_DSMPL_H * sizeof(unsigned char);
  ia->camera_size = ia->camera_width * ia->camera_height * sizeof(unsigned char);
  ia->size = ia->width * ia->height * sizeof(unsigned char);
  LOG("Will capture %d x %d frames, crop (%d) with margins of %d %d %d %d pixels, "
      "sample every %d pixels, to finally get %d x %d frames\n",
       ia->camera_width, ia->camera_height, ia->should_crop,
       ia->config.top, ia->config.right, ia->config.bottom, ia->config.left,
       ia->config.step, ia->width, ia->height);
  return 0;
}

void set_camera_config(camera_config_t *_config)
{
  desired_camera_config = *_config;
  _should_set_camera_config = 1;
}

int _set_camera_config(camera_config_t *_config)
{
  LOG("Should set camera config to margin = [%d %d %d %d], step = %d, format = %d, target = %d, ae = %d, fps = %d\n",
      _config->top, _config->right, _config->bottom, _config->left,
      _config->step, _config->format, _config->target_value, _config->ae, _config->fps);
  int error = 0;
  image_acquisition_t * ia = &image_acquisition;
  error = set_size(*_config);
  if(error) goto done;
  if(ia->config.format != _config->format &&  _config->format >=0 && _config->format < 4
#ifndef DYNAMIC_CAMERA_SIZE
    && !ia->initialized)
#endif
    )
  {
    himax_set_format(&camera, _config->format);
    ia->config.format = _config->format;
  }
  if(ia->config.ae != _config->ae)
  {
    himax_enable_ae(&camera, _config->ae);
    ia->config.ae = _config->ae;
  }
  if(ia->config.target_value != _config->target_value)
  {
    himax_set_target_value(&camera, _config->target_value);
    ia->config.target_value = _config->target_value;
  }
  if(ia->config.fps != _config->fps)
  {
    himax_set_fps(&camera, _config->fps, ia->config.format);
    ia->config.fps = _config->fps;
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

// void static fix_gains();
static void enqueue_capture();

void start_camera_loop()
{
  // allocate the memory of L2 for the image buffer
#ifdef DYNAMIC_CAMERA_SIZE
  camera_buffer_size = MAX_CAMERA_SIZE;
#else
  camera_buffer = image_acquisition.camera_size;
#endif
  camera_buffer = pi_l2_malloc(camera_buffer_size);
  if (camera_buffer == NULL){
    LOG_ERROR("Failed to alloc L2 memeory of size %d\n", camera_buffer_size);
    return;
  }
  number_of_frames = 0;
  pi_buffer_init(&(image_streaming.buffer), PI_BUFFER_TYPE_L2, camera_buffer);
  enqueue_capture(NULL);
}

static void streamer_handler(void *task)
{
  if(task)
  {
    pi_task_push((pi_task_t *)task);
  }
  else{
    enqueue_capture(NULL);
  }
}

static void end_of_frame(void *task) {
  number_of_frames++;
  pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
  if(_should_set_camera_config)
  {
    _set_camera_config(&desired_camera_config);
    _should_set_camera_config = 0;
  }
  if(_should_set_stream_config)
  {
    _set_stream_config(&desired_stream_config);
    _should_set_stream_config = 0;
  }
  // LOG("New frame %03d %03d %03d %03d ...\n", camera_buffer[0], camera_buffer[1], camera_buffer[2], camera_buffer[3]);
  if(!(number_of_frames % LED_PERIOD))
    toggle_led();
  if(!image_acquisition.config.ae)
    himax_update_exposure(&camera);
  unsigned char * frame = crop(camera_buffer);
  if(image_streaming.config.on && image_streaming.streamer)
  {
    image_streaming.buffer.data = frame;
    // int size = pi_buffer_size(&buffer);
    frame_streamer_send_async(image_streaming.streamer, &(image_streaming.buffer), pi_task_callback(&image_task, streamer_handler, task));
  }
  else if(task)
  {
    pi_task_push((pi_task_t *)task);
  }
  else{
    enqueue_capture(NULL);
  }
}

static void enqueue_capture(pi_task_t * task) {
  pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
  // Do not acquire pixels below the bottom margin, so we avoid grabber bug (and speed up)
  int size = image_acquisition.camera_size - image_acquisition.config.bottom * image_acquisition.camera_width;
  pi_camera_capture_async(&camera, camera_buffer, size, pi_task_callback(&image_task, end_of_frame, task));
}

unsigned char* grab_camera_frame()
{
  pi_task_t task;
  enqueue_capture(pi_task_block(&task));
  pi_task_wait_on(&task);
  return camera_buffer;
}

int init_camera(camera_config_t _camera_config, stream_config_t _stream_config)
{
  if(himax_open(&camera))
    return -1;
  himax_configure(&camera, FIX_EXPOSURE_AFTER);
  if(_set_camera_config(&_camera_config))
    return -1;

  // wait the camera to setup
  // if(rt_platform() == ARCHI_PLATFORM_BOARD)
  #ifndef GVSOC
  pi_time_wait_us(1000000);
  #endif

  // LOG("L2 Image alloc\t%dB\t@ 0x%08x:\t%s", camera_size, (unsigned int) camera_buffer, camera_buffer?"Ok":"Failed");
  _set_stream_config(&_stream_config);
  LOG("Initialized Himax camera\n");
  return 0;
}

void close_camera()
{
  //TODO (GAPSDK): should allow to free streamer
  himax_close(&camera);
  if(camera_buffer_size && camera_buffer)
  {
    pi_l2_free(camera_buffer, camera_buffer_size);
  }
  LOG("Closed camera\n");
}
