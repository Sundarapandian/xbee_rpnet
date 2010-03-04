#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>

#include "arch/uart.h"

/**
 * Exports for top layer interface
 **/
void * h_iface[MAX_IFACE];
int nr_iface;

/**
 * UART initialize function
 * @dev    : Device node file name example: /dev/ttyS0
 * Returns : On success - Interface ID for the corresponding UART
 *         : -1 - On failure
 **/
int uart_init(const char * dev)
{
	int fd;
	struct termios tios;

	dbg("Initializing interface %d...", nr_iface);

	if (nr_iface >= MAX_IFACE) {
		err("Invalid interface %d should be less MAX supported is %d\n", 
				nr_iface, MAX_IFACE-1);
		return -1;
	}

   	if ((fd = open(dev, O_RDWR | O_NOCTTY)) < 0) {
		err("Unable to open %s: %s\n", dev, strerror(errno));
		return -1;
	}
	if (tcgetattr(fd, &tios) < 0) {
		err("Unable to get attr for %s: %s\n", dev, strerror(errno));
		close (fd);
		return -1;
	}
	cfmakeraw(&tios);
	cfsetospeed(&tios, SYSTEM_BAUD);
	cfsetispeed(&tios, SYSTEM_BAUD);
	tios.c_cc[VMIN] = 100;
	tios.c_cc[VTIME] = 1;
	if (tcsetattr(fd, TCSANOW, &tios) < 0) {
		err("Unable to setattr for %s: %s\n", dev, strerror(errno));
		return -1;
	}
	h_iface[nr_iface] = (void *) fd;
	nr_iface ++;

	dbg("Done\n");
	return nr_iface-1;
}

/**
 * Closes all the UART interfaces
 **/
int uart_uninit(void)
{
	int i;

	dbg("Closing all uart interfaces.\n");
	for (i = 0; i < nr_iface; i++)
		close((int)h_iface[i]);
	/**
	 * FIXME: For now we are not worried about restoring
	 * attributes of the UART. It is immeterial.
	 **/
	return 0;
}

/**
 * Sends out a single character to UART interface
 **/
int uart_xmit_char(void * handle, uint8_t ch)
{
	return write((int)handle, &ch, 1);
}

/**
 * Reads a buffer of bytes from UART
 **/
int uart_recv_buffer(void * handle, uint8_t * buf, int size)
{
	return read((int)handle, buf, size);
}
