#ifndef _COMMON_H
#define _COMMON_H

#include "arch/config.h"

/**
 * XBEE module configuration options
 **/
/* Never ever ever... change the below value */
#define MAX_DL_WINDOW_SZ  72

/* Setting this will enable API MODE2 with Escaped characters */
#undef API_MODE2

#ifndef _BIT
#define _BIT(x) (1U << (x))
#endif

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;

#include <stdio.h>
#define err(a...) fprintf(stderr, "ERROR: " a)
#define dbg(a...) fprintf(stdout, "DBG: " a)

#endif /* ndef _COMMON_H */
