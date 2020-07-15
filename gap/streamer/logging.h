#ifndef __COMMON_H__
#define __COMMON_H__

#include "pmsis.h"
#include "uart.h"

/**
 * This header defines convenience macros for logging to JTAG and UART
 * Define/undefine `ENABLE_UART_LOG` and `ENABLE_JTAG_LOG` to enable the respective logging.
 *
 * By default LOG("Msg") will be printed as `[GAP8 INFO]: Msg`
 * and LOG_ERROR("Msg") as `[GAP8 ERROR]: Msg`
 * Author: Jérôme Guzzi (jerome@idsia.ch)
 * License: MIT
 */

#define ENABLE_UART_LOG
#define ENABLE_JTAG_LOG

#define INFO_PREFIX "[GAP8 INFO]: "
#define ERROR_PREFIX "[GAP8 ERROR]: "

#if defined(ENABLE_UART_LOG) && defined(UART_COMM)
#define LOG_UART(...) uart_printf (INFO_PREFIX __VA_ARGS__ )
#define LOG_ERROR_UART(...) uart_printf (ERROR_PREFIX __VA_ARGS__ )
#else
#define LOG_UART(...)
#define LOG_ERROR_UART(...)
#endif

#ifdef ENABLE_JTAG_LOG
#define LOG_JTAG(...) printf (INFO_PREFIX __VA_ARGS__ )
#define LOG_ERROR_JTAG(...) printf (ERROR_PREFIX __VA_ARGS__ )
#else
#define LOG_JTAG(...)
#define LOG_ERROR_JTAG(...)
#endif

#define LOG(...) \
{\
  LOG_UART ( __VA_ARGS__ );\
  LOG_JTAG ( __VA_ARGS__ );\
}

#define LOG_ERROR(...) \
{\
  LOG_ERROR_UART ( __VA_ARGS__ );\
  LOG_ERROR_JTAG ( __VA_ARGS__ );\
}

#endif // __COMMON_H__
