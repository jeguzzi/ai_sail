#include "pmsis.h"
#include "stdio.h"
#include "led.h"
#include "camera.h"
#include "uart.h"
#include "nina.h"
#include "common.h"

int main()
{
  if(init_uart(1))
  {
    pmsis_exit(-1);
  }
  init_nina(0);
  camera_config_t camera_config = {
    .top = 33,
    .right = 2,
    .bottom = 33,
    .left = 0,
    .format = HALF,
    .step = 1,
    .target_value = 0x60
  };
  stream_config_t stream_config = {
    .on = 0,
    .format = 0,
    .transport = TRANSPORT_WIFI
  };
  if(init_camera(camera_config, stream_config))
  {
    pmsis_exit(-1);
  }
  init_led();
  start_camera_loop();
  LOG("Starting main loop\n");
  while(1)
  {
    pi_yield();
  }
  close_camera();
  close_nina();
  close_uart();
  return 0;
}
