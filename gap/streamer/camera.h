#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "pmsis.h"
#include "frame_streamer_utils.h"
#include "uart_protocol.h"
#include "himax_utils.h"

#define DYNAMIC_IMAGE_SIZE
#define DYNAMIC_CAMERA_SIZE

typedef struct{
  struct pi_device *device;
  int camera_width;
  int camera_height;
  int cropped_width;
  int cropped_height;
  int width;
  int height;
  int camera_size;
  int size;
  int should_crop;
  int initialized;
  camera_config_t config;
} image_acquisition_t;

typedef struct{
  frame_streamer_t *streamer;
  pi_buffer_t buffer;
  stream_config_t config;
} image_streaming_t;


// needed also to be able to change binning while running
#define FIX_EXPOSURE_AFTER 10
#define TRANSPORT_WIFI 0
// #define TRANSPORT_PIPE 1
#define LED_PERIOD 10
int init_camera(camera_config_t, stream_config_t);
unsigned char* grab_camera_frame();
void close_camera();
void start_camera_loop();


#define default_camera_config(...) ((camera_config_t){.step=1, .ae=1, .target_value=80, .fps=20, .format=HALF, .bottom=1, .top=1, __VA_ARGS__})
#define default_stream_config(...) ((stream_config_t){.transport = TRANSPORT_WIFI, __VA_ARGS__})


// just exposed for testing
// TODO encapsulate better
extern image_acquisition_t image_acquisition;
extern image_streaming_t image_streaming;
unsigned char * crop(unsigned char *);

#endif // __CAMERA_H__
