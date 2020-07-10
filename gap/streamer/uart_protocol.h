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
} __attribute__((packed)) camera_config_t;

#define CAMERA_HEADER "!CAM"
void send_camera_config(camera_config_t *config);
// To be implemented if CAMERA_HEADER is defined
void set_camera_config(camera_config_t *config);


typedef struct {
  uint8_t on;
  uint8_t format;
  uint8_t transport;
} __attribute__((packed)) stream_config_t;

#define STREAM_HEADER "!STR"
void send_stream_config(stream_config_t *config);
// To be implemented if STREAM_HEADER is defined
void set_stream_config(stream_config_t *config);

#endif
