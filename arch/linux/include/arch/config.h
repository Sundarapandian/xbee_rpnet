#ifndef _CONFIG_H
#define _CONFIG_H

/**
 * XBEE module configuration options
 **/
/* Never ever ever... change the below value */
#define MAX_DL_WINDOW_SZ  72

#ifndef _BIT
#define _BIT(x) (1U << (x))
#endif

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;

#include <stdio.h>
#define err(a...) fprintf(stderr, "ERROR: " a)
#define dbg(a...) fprintf(stdout, "DBG: " a)

#include <arch/uart.h>

#endif /* ndef _CONFIG_H */
