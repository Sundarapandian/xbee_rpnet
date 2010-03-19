#ifndef _UART_H
#define _UART_H

#include "arch/common.h"

#define UART_WAIT_FOREVER    0
#define TIMER_MS(x)          ((x)/10)

#ifdef __cplusplus
extern "C" {
#endif
	int uart_init(const char * dev);
	int uart_uninit(void);
	int uart_xmit_char(void *handle, uint8_t ch);
	int uart_getchar(void * handle,  int timeout);
#ifdef __cplusplus
}
#endif
#endif /* ndef _UART_H */
