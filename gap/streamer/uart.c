#include "uart.h"
#include "logging.h"

#define CONFIG_HYPERBUS_DATA6_PAD      ( PI_PAD_46_B7_SPIM0_SCK )
// This is due to an HW bug to be fixed in future chips
#define CONFIG_UART_RX_PAD_FUNC        ( 0 )
#define CONFIG_HYPERRAM_DATA6_PAD_FUNC ( 3 )

extern int _prf(int (*func)(), void *dest, const char *format, va_list vargs);

static struct pi_device device;
static int initialized = 0;
static int tx_initialized = 0;
static int tx_enabled = 0;

// Autogenerate private implementation of the uart protocol
#include "uart_protocol.c"

int init_uart(int enable_rx) {
  struct pi_uart_conf conf;
  pi_uart_conf_init(&conf);
  conf.baudrate_bps = 115200;
  conf.enable_tx = 1;
  conf.enable_rx = 1;
  pi_open_from_conf(&device, &conf);
  if (pi_uart_open(&device))
  {
    printf("[GAP8 Error]: Failed to open UART\n");
    return -1;
  }
  initialized = 1;
  //start listening
  if(enable_rx)
  {
#ifdef UART_RX_PROTOCOL
    tx_initialized = 1;
    set_rx_enabled(1);
    start_rx_protocol();
    LOG("Started UART RX\n");
#endif
  }
  LOG("Initialized UART\n");
  return 0;
}

static int uart_tfp_putc(int c, void *dest) {
  pi_uart_write(&device, (uint8_t *)&c, 1);
  return c;
}

int uart_printf(const char *fmt, ...) {
  if(!initialized) return 0;
  va_list va;
  va_start(va, fmt);
  _prf(uart_tfp_putc, NULL, fmt, va);
  va_end(va);
  return 0;
}

void uart_flush_rx() {
  if(!initialized) return;
  // pi_uart_ioctl(&device, PI_UART_IOCTL_FLUSH, NULL);
  // pi_uart_ioctl(&device, PI_UART_IOCTL_ABORT_RX, NULL);
  // pi_uart_ioctl(&device, PI_UART_IOCTL_ENABLE_RX, NULL);
  // LOG("Flushed UART RX");
}

void close_uart() {
  if(initialized) {
    pi_uart_close(&device);
    initialized = 0;
    LOG("Closed UART\n");
  }
}

void set_rx_enabled(int value) {
  if(value == tx_enabled || !tx_initialized) return;
  tx_enabled = value;
  if(value) {
    pi_pad_set_function(CONFIG_HYPERBUS_DATA6_PAD, CONFIG_UART_RX_PAD_FUNC);
  }
  else {
    pi_pad_set_function(CONFIG_HYPERBUS_DATA6_PAD, CONFIG_HYPERRAM_DATA6_PAD_FUNC);
  }
}
