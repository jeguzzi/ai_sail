#include "pmsis.h"
#include "led.h"
#include "logging.h"

#define LED_GPIO 2

static struct pi_device gpio_device;
static uint32_t led_val = 0;

void init_led()
{
  pi_gpio_pin_configure(&gpio_device, LED_GPIO, PI_GPIO_OUTPUT);
  LOG("Initialized LED\n");
}

static void update_led(){
  pi_gpio_pin_write(&gpio_device, LED_GPIO, led_val);
}

void set_led(int value){
  led_val = (uint32_t) value;
  pi_gpio_pin_write(&gpio_device, LED_GPIO, led_val);
}

void toggle_led()
{
  led_val ^= 1;
  update_led();
}
