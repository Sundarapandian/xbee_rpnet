#ifndef _UART_H
#define _UART_H

#define UART_WAIT_FOREVER    0
#define TIMER_MS(x)          ((x)/10)

/* Interface mappings to UART functions */
#define iface_putchar(x,ch) uart_putchar(x,ch)
#define iface_getchar(x)    uart_getchar(x,UART_WAIT_FOREVER)
#define iface_getchar_timeout(x,tout) uart_getchar(x,tout)

#define SYSTEM_BAUD       B9600 /* Systemwise Baudrate */
#define MAX_IFACE         1     /* Maximum number of interfaces */

#ifdef __cplusplus
extern "C" {
#endif
	int uart_init(const char * dev);
	int uart_uninit(void);
	int uart_putchar(void *handle, uint8_t ch);
	int uart_getchar(void * handle,  int timeout);
#ifdef __cplusplus
}
#endif
#endif /* ndef _UART_H */
