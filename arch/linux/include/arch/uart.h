#ifndef _UART_H
#define _UART_H

#include "arch/common.h"

#ifdef __cplusplus
extern "C" {
#endif
	int uart_init(const char * dev);
	int uart_uninit(void);
	int uart_xmit_char(void *handle, uint8_t ch);
	int uart_recv_buffer(void * handle, uint8_t * buf, int size);
#ifdef __cplusplus
}
#endif
#endif /* ndef _UART_H */
