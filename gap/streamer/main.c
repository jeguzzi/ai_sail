#include "pmsis.h"
#include "stdio.h"
#include "led.h"
#include "camera.h"
#include "uart.h"
#include "nina.h"
#include "common.h"

extern void PMU_set_voltage(int, int);

int main()
{
  PMU_set_voltage(1000, 0);
  pi_time_wait_us(10000);
  pi_freq_set(PI_FREQ_DOMAIN_FC, 100000000);
  pi_time_wait_us(10000);
  pi_freq_set(PI_FREQ_DOMAIN_CL, 100000000);
  pi_time_wait_us(10000);

  pi_time_wait_us(1000000);

#ifdef UART_COMM
  if(init_uart(1))
  {
    pmsis_exit(-1);
  }
  set_rx_enabled(0);
#endif
  init_nina(0);
  // camera_config_t camera_config = {
  //   .top = 33,
  //   .right = 2,
  //   .bottom = 33,
  //   .left = 0,
  //   .format = HALF,
  //   .step = 1,
  //   .target_value = 0x60,
  //   .fps = 10
  // };
  // stream_config_t stream_config = {
  //   .on = 0,
  //   .format = 0,
  //   .transport = TRANSPORT_WIFI
  // };
  if(init_camera(default_camera_config(.top=33, .right=2, .bottom=33, .left=0),
                 default_stream_config()))
  {
    pmsis_exit(-1);
  }
  init_led();
  start_camera_loop();
  LOG("Starting main loop\n");
#ifdef UART_COMM
  set_rx_enabled(1);
#endif
  while(1)
  {
    pi_yield();
  }
  close_camera();
  close_nina();
#ifdef UART_COMM
  close_uart();
#endif
  return 0;
}
