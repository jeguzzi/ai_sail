#include "pmsis.h"
#include "nina.h"
#include "logging.h"
#include "bsp/transport/nina_w10.h"

#define NINA_GPIO_OUT  PI_GPIO_A18_PAD_32_A13

static struct pi_device gpio_device;
static struct pi_device wifi;

static void wake_up_nina()
{
  // LOG("Wake up NINA\n");
  pi_gpio_pin_configure(&gpio_device, NINA_GPIO_OUT, PI_GPIO_OUTPUT);
  pi_gpio_pin_write(&gpio_device, NINA_GPIO_OUT, 1);
}

static void put_nina_to_sleep()
{
  // LOG("Put NINA to sleep\n");
  pi_gpio_pin_write(&gpio_device, NINA_GPIO_OUT, 0);
}

struct pi_device * open_wifi()
{
  wake_up_nina();
  pi_time_wait_us(1000);

  struct pi_nina_w10_conf nina_conf;
  pi_nina_w10_conf_init(&nina_conf);
  // cannot be left empty :-/ !!!
  nina_conf.ssid = "";
  nina_conf.passwd = "";
  nina_conf.ip_addr = "127.0.0.1";
  nina_conf.port = 5555;
  pi_open_from_conf(&wifi, &nina_conf);
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
