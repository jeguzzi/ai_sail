/*
  This file was autogenerate on 07/15/20 from the following Python config:

  from base import config, c_uint8, c_uint16  # type: ignore


  @config(name='camera_config', group='aideck_HIMAX', header="!CAM")
  class Camera:
      top: c_uint16
      right: c_uint16
      bottom: c_uint16
      left: c_uint16
      format: c_uint8  # noqa
      step: c_uint8
      target_value: c_uint8
      ae: c_uint8
      fps: c_uint8


  @config(name='stream_config', group='aideck_stream', header="!STR")
  class Stream:
      on: c_uint8
      format: c_uint8  # noqa
      transport: c_uint8

*/

#ifndef __UART_PROTOCOL_H__
#define __UART_PROTOCOL_H__
#include <stdint.h>

typedef struct {
  uint16_t top;
  uint16_t right;
  uint16_t bottom;
  uint16_t left;
  uint8_t format;
  uint8_t step;
  uint8_t target_value;
  uint8_t ae;
  uint8_t fps;
} __attribute__((packed)) camera_config_t;

#define CAMERA_CONFIG_HEADER "!CAM"
void send_camera_config(camera_config_t *config);
// To be implemented if CAMERA_CONFIG_HEADER is defined
#ifdef CAMERA_CONFIG_HEADER
void set_camera_config(camera_config_t *config);
#endif


typedef struct {
  uint8_t on;
  uint8_t format;
  uint8_t transport;
} __attribute__((packed)) stream_config_t;

#define STREAM_CONFIG_HEADER "!STR"
void send_stream_config(stream_config_t *config);
// To be implemented if STREAM_CONFIG_HEADER is defined
#ifdef STREAM_CONFIG_HEADER
void set_stream_config(stream_config_t *config);
#endif

#endif // __UART_PROTOCOL_H__