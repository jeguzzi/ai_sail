#include "pmsis.h"
#include "nina.h"
#include "common.h"
#include "bsp/transport/nina_w10.h"

#define NINA_GPIO_OUT  PI_GPIO_A18_PAD_32_A13

static struct pi_device gpio_device;
static struct pi_device wifi;

static void wake_up_nina()
{
  // printf("will wake_up_nina\n");
  pi_gpio_pin_configure(&gpio_device, NINA_GPIO_OUT, PI_GPIO_OUTPUT);
  pi_gpio_pin_write(&gpio_device, NINA_GPIO_OUT, 1);
  // printf("did wake_up_nina\n");
}

static void put_nina_to_sleep()
{
  // printf("will put_nina_to_sleep\n");
  pi_gpio_pin_write(&gpio_device, NINA_GPIO_OUT, 0);
  // printf("did put_nina_to_sleep\n");
}

struct pi_device * open_wifi()
{
  // LOG("Wake up NINA\n");
  wake_up_nina();
  pi_time_wait_us(1000);
  // LOG("Open WiFi\n");

  struct pi_nina_w10_conf nina_conf;
  pi_nina_w10_conf_init(&nina_conf);
  // cannot be left empty :-/ !!!
  nina_conf.ssid = "Hasse";
  nina_conf.passwd = "AngelicasIphone";
  nina_conf.ip_addr = "192.168.1.112";
  nina_conf.port = 5555;
  // nina_conf.ip_addr = "192.168.201.40";
  // printf("Will pi_open_from_conf\n");
  pi_open_from_conf(&wifi, &nina_conf);
  // printf("Did pi_open_from_conf\n");
  // printf("Will pi_open_from_conf\n");
  if (pi_transport_open(&wifi))
  {
    LOG_ERROR("Failed to open wifi\n");
    return NULL;
  }
  LOG("Opened wifi\n");
  // printf("Did pi_open_from_conf\n");
  return &wifi;
}


void init_nina(int active)
{
  LOG("Initialized NINA (GPIO)\n");
  pi_gpio_pin_configure(&gpio_device, NINA_GPIO_OUT, PI_GPIO_OUTPUT);
  pi_gpio_pin_write(&gpio_device, NINA_GPIO_OUT, active);
}

void close_nina()
{
  LOG("Closed wifi\n");
  // Disabled to facilitate developement
  // put_nina_to_sleep();
}
