/**
 * This header define higher-level and helpers interfaces to work with UART.
 * The actual protocol header `uart_procol.h` is automatically generated.
 * Author: Jérôme Guzzi (jerome@idsia.ch)
 * License: MI
 */
#ifndef __UART_H__
#define __UART_H__

/**
 * Initialize the UART
 * @param  enable_tx launches the asynchronous loop to listen for incoming [protocol] messages.
 * @return           A value different than 0 in case of failure.
 */
int init_uart(int enable_tx);

/**
 * Close the UART
 */
void close_uart();

/**
 * A printf-like function to print strings over UART
 * @param  fmt     The format
 * @param  VARARGS The values
 * @return         Same as printf.
 */
int uart_printf(const char *fmt, ...);

/**
 * A function to flush the uart in case of problems.
 */
void uart_flush_rx();

/**
 * Toggle the state of UART TX. It is only effective if it was enabled in `init_uart`.
 * This is needed to overcome the hardware bug that does not let you use UART TX and HYPERRAM
 * at the same time. So if you need both, you are expected to guard any function needing `HYPERRAM`
 * with set_rx_enabled(0); your_hyperram_fnc(); set_rx_enabled(1);
 * @param value The state of UART RX (0 to disable, any other value to enable)
 */
void set_rx_enabled(int value);

#endif // __UART_H__
