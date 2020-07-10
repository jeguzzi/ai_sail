#include "uart_protocol.h"

#define HEADER_LENGTH 4
#define BUFFER_LENGTH 11
#define UART_RX_PROTOCOL
static PI_L2 uint8_t header[HEADER_LENGTH];
static PI_L2 uint8_t rx_buffer[BUFFER_LENGTH];
static pi_task_t task;
void send_camera_config(camera_config_t *config)
{
  if(!enabled) return;
#ifdef CAMERA_HEADER
  pi_uart_write(&device, CAMERA_HEADER, HEADER_LENGTH);
  pi_uart_write(&device, (uint8_t *)config, sizeof(camera_config_t));
#endif
}

void send_stream_config(stream_config_t *config)
{
  if(!enabled) return;
#ifdef STREAM_HEADER
  pi_uart_write(&device, STREAM_HEADER, HEADER_LENGTH);
  pi_uart_write(&device, (uint8_t *)config, sizeof(stream_config_t));
#endif
}


static void received_header(void *arg)
{

#ifdef CAMERA_HEADER
  if(memcmp(header, CAMERA_HEADER, HEADER_LENGTH) == 0)
  {
    if(pi_uart_read(&device, rx_buffer, sizeof(camera_config_t)))
    {
      LOG_ERROR("Failed to read camera config\n");
    }
    else{
      set_camera_config((camera_config_t *)rx_buffer);
    }
    goto done;
  }
#endif

#ifdef STREAM_HEADER
  if(memcmp(header, STREAM_HEADER, HEADER_LENGTH) == 0)
  {
    if(pi_uart_read(&device, rx_buffer, sizeof(stream_config_t)))
    {
      LOG_ERROR("Failed to read stream config\n");
    }
    else{
      set_stream_config((stream_config_t *)rx_buffer);
    }
    goto done;
  }
#endif
  done:
  pi_uart_read_async(&device, header, HEADER_LENGTH, &task);
}

static void start_rx_protocol()
{
  pi_task_callback(&task, (void *) received_header, &task);
  pi_uart_read_async(&device, header, HEADER_LENGTH, &task);
}