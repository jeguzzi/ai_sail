#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "pmsis.h"

#define QVGA 0   // 324 x 244
#define QQVGA 1  // 162 x 122
#define FULL 2   // 324 x 324
#define HALF 3   // 162 x 162

// #define DEBUG_AE
// needed also to be able to change binning while running
#define FIX_GAINS 10
#define TRANSPORT_WIFI 0
#define TRANSPORT_PIPE 1

typedef struct StreamConfig
{
  uint8_t on;
  uint8_t format; // 0 RAW, 1 JPEG
  uint8_t transport; // 0 WIFI, 1 PIPE
} __attribute__((packed)) stream_config_t;

typedef struct CameraConfig
{
  uint16_t top;
  uint16_t right;
  uint16_t bottom;
  uint16_t left;
  uint8_t format;
  uint8_t step;
  uint8_t target_value;
} __attribute__((packed)) camera_config_t;

int init_camera(camera_config_t, stream_config_t);
unsigned char* grab_camera_frame();
void close_camera();
void start_camera_loop();

#endif // __CAMERA_H__
