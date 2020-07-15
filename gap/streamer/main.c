/**
 * The main file for AI SAIL (camera streaming via pipe/wifi from the AI-DECK)
 *
 * Author: Jérôme Guzzi (jerome@idsia.ch)
 * License: MIT
 */

#include "pmsis.h"
#include "logging.h"
#include "uart.h"
#include "led.h"
#include "camera.h"
#include "nina.h"


int main()
{
  pi_freq_set(PI_FREQ_DOMAIN_FC, 100000000);
  pi_time_wait_us(10000);
  pi_freq_set(PI_FREQ_DOMAIN_CL, 100000000);
  pi_time_wait_us(1000000);

  if(init_uart(1)) {
    pmsis_exit(-1);
  }
  set_rx_enabled(0);
  init_nina(0);
  if(init_camera(default_camera_config(.top=33, .right=2, .bottom=33, .left=0, .fps=30),
                 default_stream_config(.on=1))) {
    pmsis_exit(-1);
  }
  init_led();
  // We do not need the camera frame: let the camera loop run by itself
  start_camera_loop(false, false);
  LOG("Starting main loop\n");
  set_rx_enabled(1);
  while(1) {
    pi_yield();
  }
  close_camera();
  close_nina();
  close_uart();
  return 0;
}
