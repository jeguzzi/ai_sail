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

#include "uart_protocol.h"

#define HEADER_LENGTH 4
static PI_L2 uint8_t header[HEADER_LENGTH];


#define UART_RX_PROTOCOL
#define BUFFER_LENGTH 13
static PI_L2 uint8_t rx_buffer[BUFFER_LENGTH];
static pi_task_t task;

void send_camera_config(camera_config_t *config)
{
  if(!initialized) return;
#ifdef CAMERA_CONFIG_HEADER
  pi_uart_write(&device, CAMERA_CONFIG_HEADER, HEADER_LENGTH);
  pi_uart_write(&device, (uint8_t *)config, sizeof(camera_config_t));
#endif
}

void send_stream_config(stream_config_t *config)
{
  if(!initialized) return;
#ifdef STREAM_CONFIG_HEADER
  pi_uart_write(&device, STREAM_CONFIG_HEADER, HEADER_LENGTH);
  pi_uart_write(&device, (uint8_t *)config, sizeof(stream_config_t));
#endif
}

static void received_header(void *arg) {

#ifdef CAMERA_CONFIG_HEADER
  if(memcmp(header, CAMERA_CONFIG_HEADER, HEADER_LENGTH) == 0) {
    if(pi_uart_read(&device, rx_buffer, sizeof(camera_config_t))) {
      LOG_ERROR("Failed to read camera_config config\n");
    }
    else {
      set_camera_config((camera_config_t *)rx_buffer);
    }
    goto done;
  }
#endif

#ifdef STREAM_CONFIG_HEADER
  if(memcmp(header, STREAM_CONFIG_HEADER, HEADER_LENGTH) == 0) {
    if(pi_uart_read(&device, rx_buffer, sizeof(stream_config_t))) {
      LOG_ERROR("Failed to read stream_config config\n");
    }
    else {
      set_stream_config((stream_config_t *)rx_buffer);
    }
    goto done;
  }
#endif

  done:
  pi_uart_read_async(&device, header, HEADER_LENGTH, &task);
}

static void start_rx_protocol() {
  pi_task_callback(&task, (void *) received_header, &task);
  pi_uart_read_async(&device, header, HEADER_LENGTH, &task);
}