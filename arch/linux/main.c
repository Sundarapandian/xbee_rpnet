#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "xbee/psock.h"
#include "xbee/api.h"
#include "arch/uart.h"

static volatile int end;
static pthread_t t_rx, t_app;
#define MAX_ERR      10
#define IN_PIPE_NAME "in_xbee"
#define close_all_threads() do { \
			pthread_cancel(t_rx); \
			pthread_cancel(t_app); \
		}while(0)

/**
 * Right now allocated in stack
 * Don't change it to a crazy value
 **/
#define RX_BUF_SZ    100
#define CMD_BUF_SZ   100

static void app_sig_handler(int sig_id)
{
	end = 1;
	pthread_cancel(t_rx);
	pthread_cancel(t_app);
}

static int rx_print(const struct psock * ps)
{
	int val, i;
	uint8_t buf[RX_BUF_SZ];

	val = dl_recv_frame(ps, buf, sizeof buf);
	if (val <= 0) return 0; /* Let caller deal with err */

	dbg ("Rx_Thread: Received %d bytes from uart!\n", val);
	for (i = 0; i < val; i++) {
		printf ("%02X ", buf[i]);
	}
	printf ("\n");

	return val;
}

/**
 * Listening thread! Listening will happen mainly on a local interface
 * Hence a local socket listen should suffice!
 **/
static void * rx_thread(void *arg)
{
	int err = 0;
	struct psock ps;

	/**
	 * Create a local socket to listen on
	 * We need only the interface parameter
	 * which we will get from the caller!
	 **/
	if (psock_local((int)arg, &ps, 0) == NULL) {
		err("Unable to open local socket.\n");
		exit(EXIT_FAILURE);
	}

	while (!end) {
		int ret = rx_print(&ps);
		if (ret && !(err = 0)) continue;
		err ++;
		if (err >= MAX_ERR) {
			err ("FATAL: Too many Zero frames or Rx errors! Quitting!\n");
			end = 1;
			exit(EXIT_FAILURE);
		}
	}
	pthread_exit(EXIT_SUCCESS);
}

static struct psock ps_local, ps_bcast;

void app_handle_cmd (char * buf)
{
	struct psock * ps = NULL;
	dbg("Received cmd %s\n", buf);
	switch (buf[0]) {
		case 'X': /* Exit application */
			close_all_threads();
			break;
		case 'R': /* Remote AT command */
			ps = &ps_bcast;
		case 'L': /* Local AT command */
			if (ps == NULL) ps = &ps_local;
			dl_send_at_command(ps, (uint8_t *) &buf[2]);
			break;
		case 'D': /* Data packet */
			dl_tx_data(&ps_bcast, (uint8_t *) &buf[2], strlen(&buf[2]));
			break;
		default:
			err("Invalid command: %C\n", buf[0]);
	}

}

static void * app_thread(void * arg)
{
	FILE * pf;
	char cmd_buf[CMD_BUF_SZ];

	unlink(IN_PIPE_NAME);
	if (mkfifo(IN_PIPE_NAME, 0644) < 0) {
		err("Unable to created named input pipe %s:%s\n",
			   IN_PIPE_NAME, strerror(errno));
		exit(EXIT_FAILURE);
	}

	pf = fopen(IN_PIPE_NAME, "r");
	if (pf == NULL) {
		err("Unable to open input pipe %s:%s\n",
			   IN_PIPE_NAME, strerror(errno));
		exit(EXIT_FAILURE);
	}
	/* Initialize global sockets */
	//psock_init(&ps_bcast, iface, 0x0013A200, 0x4033234C, ADDR16_ANY, 0, 0, 0, PSOCK_PROFILE_DEFAULT, 0);
	psock_init(&ps_bcast, (int) arg, ADDR64_BROADCAST_HI, ADDR64_BROADCAST_LO,
			ADDR16_ANY, PS_EP_DATA, PS_EP_DATA, PS_CID_LOOPBACK, PSOCK_PROFILE_DEFAULT, 0);
	psock_local((int) arg, &ps_local, 0);

	while(!end) {
		int len;
		if(fgets(cmd_buf, sizeof cmd_buf, pf) == NULL) {
			if ((pf = fopen(IN_PIPE_NAME, "r")) == NULL) {
				err("Borken pipe. Quitting!\n");
				exit(EXIT_FAILURE);
			} else
				continue;
		}
		len = strlen(cmd_buf);

		/* Remove any CR-LF at the end of string if exist */
		if (cmd_buf[len - 1] == '\n' || cmd_buf[len - 1] == '\r')
			cmd_buf[len - 1] = '\0';
		if (cmd_buf[len - 2] == '\n' || cmd_buf[len - 2] == '\r')
			cmd_buf[len - 2] = '\0';
		app_handle_cmd(cmd_buf);
	}
	pthread_exit(NULL);
}

int main(int cnt, char *vec[])
{
	int iface;

	if (cnt < 2) {
		err("Usage:\n"
			"%s <UART device>\n"
			"UART device --> UART device node Ex: /dev/ttyS0\n",
			vec[0]);
		return EXIT_FAILURE;
	}

	signal(SIGTERM, app_sig_handler);
	signal(SIGINT, app_sig_handler);

	if ((iface = uart_init(vec[1])) < 0) {
		return EXIT_FAILURE;
	}

	if (pthread_create(&t_app, NULL, app_thread, (void *)iface)) {
		err("Unable to create application thread\n");
		return EXIT_FAILURE;
	}

	if (pthread_create(&t_rx, NULL, rx_thread, (void *)iface)) {
		err("Unable to create RX thread [%s]\n", strerror(errno));
		return EXIT_FAILURE;
	}
	pthread_join(t_rx, NULL);
	pthread_join(t_app, NULL);

	unlink(IN_PIPE_NAME);
	dbg ("APP: Exiting...\n");
	return EXIT_SUCCESS;
}
