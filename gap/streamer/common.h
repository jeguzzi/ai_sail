#ifndef __COMMON_H__
#define __COMMON_H__

#include "pmsis.h"
#include "uart.h"

#define VERBOSE

#ifdef UART_COMM
// #define LOG_UART
#endif
#define LOG_JTAG

#define ABS(x)   ( (x<0) ? (-x) : x )
#define MAX(x,y) ( (x>y) ? x : y )
#define MIN(x,y) ( (x<y) ? x : y )

#if defined(LOG_UART) && defined(LOG_JTAG)
#define LOG(...) \
{\
  uart_printf ("[GAP8 INFO]: "__VA_ARGS__ );\
  printf ("[GAP8 INFO]: "__VA_ARGS__ );\
}

#define LOG_ERROR(...) \
{\
  uart_printf ("[GAP8 ERROR]: "__VA_ARGS__ );\
  printf ("[GAP8 ERROR]: "__VA_ARGS__ );\
}
#elif defined(LOG_UART)
#define LOG(...) uart_printf ("[GAP8 INFO]: "__VA_ARGS__ )
#define LOG_ERROR(...) uart_printf ("[GAP8 ERROR]: "__VA_ARGS__ )
#elif defined(LOG_JTAG)
#define LOG(...) printf ("[GAP8 INFO]: "__VA_ARGS__ )
#define LOG_ERROR(...) printf ("[GAP8 ERROR]: "__VA_ARGS__ )
#else
#define LOG(...)
#define LOG_ERROR(...)
#endif

#endif // __COMMON_H__
