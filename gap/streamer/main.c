#include "pmsis.h"
#include "stdio.h"
#include "led.h"
#include "camera.h"
#include "uart.h"
#include "nina.h"
#include "common.h"
#include "bsp/bsp.h"

// static struct pi_device wifi;

// static int open_wifi(struct pi_device *device)
// {
//   struct pi_nina_w10_conf nina_conf;
//   pi_nina_w10_conf_init(&nina_conf);
//   // cannot be left empty :-/ !!!
//   nina_conf.ssid = "Hasse";
//   nina_conf.passwd = "AngelicasIphone";
//   nina_conf.ip_addr = "192.168.1.112";
//
//   nina_conf.port = 5555;
//
//   // nina_conf.ip_addr = "192.168.201.40";
//   pi_open_from_conf(device, &nina_conf);
//   if (pi_transport_open(device))
//     return -1;
//   return 0;
// }

// static pi_task_t timer_task;
//
// static void timer_handle(void *arg)
// {
//   // toggle_led();
//   pi_task_push_delayed_us(pi_task_callback(&timer_task, timer_handle, NULL), 500000);
// }

int main()
{
  bsp_init();
  // with the default PADS config UART RX won't work.
  // it would be better to call the specific function ...
  // pi_pad_set_function(...) but I don't know the arguments
  uint32_t pads_value[] = {0x00055500, 0x0f000000, 0x003fcfff, 0x00000000};
  pi_pad_init(pads_value);
  if(init_uart(1))
  {
    pmsis_exit(-1);
  }
  // printf("Will init_nina\n");
  init_nina(0);
  // printf("Did init_nina\n");
  // printf("Will init_camera\n");
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
  // printf("Did init_camera\n");
  // printf("Will open_wifi\n");
  // if (open_wifi(&wifi))
  // {
  //   printf("Failed to open wifi\n");
  //   return -1;
  // }
  // printf("Did open_wifi\n");
  // printf("Will init_streamer\n");
  // if(init_streamer(&wifi))
  // {
  //   printf("Failed to open stream\n");
  //   pmsis_exit(-1);
  // }
  // printf("Did init_streamer\n");
  // pi_time_wait_us(100000);
  init_led();
  // pi_task_push_delayed_us(pi_task_callback(&timer_task, timer_handle, NULL), 500000);
  // uint8_t byte = 1;
  // int i = pi_transport_send(&wifi, &byte, 1);
  // printf("P1 %d\n", i);
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
