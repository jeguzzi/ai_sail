#ifndef __UART_H__
#define __UART_H__

int init_uart(int);
int uart_printf(const char *fmt, ...);
void uart_flush_rx();
void close_uart();
void read_byte();
void set_rx_enabled(int value);

#endif // __UART_H__
