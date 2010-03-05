#include <string.h>

#include "arch/uart.h"
#include "xbee/api.h"

/* dl_parameters structure */
struct dl_param dl_param = {
	DL_PARAM_TTL_DEFAULT,
	DL_PARAM_APIMODE_DEFAULT,
	DL_PARAM_AOMODE_DEFAULT
};

/**
 * Transmits the character to PHY layer
 * It escapes the characters if required
 **/
static void phy_putchar(void * handle, uint8_t ch)
{
		if (dl_param.mode_ap == API_MODE2 && is_esc_char(ch)) {
			uart_xmit_char(handle, ESC_CHAR);
			ch ^= 0x20;
		}
		uart_xmit_char(handle, ch);
}

/**
 * Forms the top level packet for UART TX
 * and sends it to the XBEE device
 **/
static int dl_send_data(void * handle, const uint8_t * data, int size)
{
	unsigned int crc = 0, i;
	uart_xmit_char(handle, API_HEADER); /* Emit frame header */
	phy_putchar(handle, (size >> 8) & 0xFF); /* Emit MSB of size */
	phy_putchar(handle, size & 0xFF); /* Emit MSB of Size */
	for (i = 0; i < size; i++) { /* Emit LSB of size */
		uint8_t ch = data[i];
		crc += ch;
		phy_putchar(handle, ch);
	}

	phy_putchar(handle, (0xFF - crc) & 0xFF);
	return 0;
}

/**
 * Executes an AT command locally
 **/
static int dl_local_at_command(const struct psock * ps, const uint8_t * cmd)
{
	int len = strlen((char *)cmd);
	uint8_t buf[2 + len + 1];

	/* Command ID and frame number */
	buf[0] = (ps->flags & PS_FLAG_QUEUE_CHANGE) ? API_ID_ATCMD_QPV : API_ID_ATCMD;
	buf[1] = !(ps->flags & PS_FLAG_DISCARD_REPLY);

	strcpy((char *)&buf[2], (char *)cmd);
	return dl_send_data(ps->hiface, buf, len + 2);
}

/**
 * Executes an AT command on the remote side!
 **/
static int dl_remote_at_command(const struct psock * ps, const uint8_t * cmd)
{
	int len = strlen((char *)cmd);
	/* 13 = Cmd(1) + FrameID(1) + Addr64(8) + Addr16(2) + CmdOption(1) */
	uint8_t buf[13 + len + 1], *t = buf;

	*t++ = API_ID_REMOTE_ATCMD;
	*t++ = !(ps->flags & PS_FLAG_DISCARD_REPLY);
	memcpy(t, ps->addr64, 8); t += 8;
	*t++ = ps->addr16[0]; *t++ = ps->addr16[1];
	*t++ = (ps->flags & PS_FLAG_QUEUE_CHANGE) ? 0x00 : 0x02;
	strcpy((char *)t, (char *) cmd);
	return dl_send_data(ps->hiface, buf, 13 + len);
}

/**
 * Executes an AT command on LOCAL Device or REMOTE device
 * Based on type of socket!
 **/
int dl_send_at_command(const struct psock * sock, const uint8_t * cmd)
{
	unsigned int flag;
	if (sock == NULL || !is_ps_valid(sock))
		goto err;

	flag = sock->flags & (PS_FLAG_LOCAL | PS_FLAG_REMOTE);

	/* Sanity check */
	if (flag == 0 || flag == (PS_FLAG_LOCAL | PS_FLAG_REMOTE))
		goto err;

	(flag == PS_FLAG_LOCAL ? dl_local_at_command : dl_remote_at_command)(sock, cmd);
	return 0;

err:
	dbg("send_at_command invalid socket!\n");
	return -1;
}

int dl_tx_data(const struct psock * s, const uint8_t * buffer, int size)
{
	static uint8_t frame_no;
	uint8_t buf[20 + size], *ptr;

	/* Sanity Check */
	if (s == NULL || !is_ps_valid(s) || (s->flags & PS_FLAG_LOCAL)) {
		err("tx_data: Invalid socket!\n");
		return -1;
	}
	if (buffer == NULL || size > MAX_DL_WINDOW_SZ) {
		err("tx_data: Data is too big or invalid buffer\n");
		return -1;
	}
	buf[0] = API_ID_TX;
	ptr = &buf[1];
	*ptr++ = s->flags & PS_FLAG_DISCARD_REPLY ? 0 : ++frame_no;
	memcpy(ptr, s->addr64, 8); ptr += 8;
	*ptr++ = s->addr16[0];
	*ptr++ = s->addr16[1];
	if (dl_param.mode_ao == 1) {
		buf[0] = API_ID_TX_EXPLICIT;
		*ptr++ = s->sep;    /* Source Endpoint */
		*ptr++ = s->dep;    /* Destination Endpoint */
		*ptr++ = 0;         /* Reserved byte */
		*ptr++ = s->cid;    /* Cluster ID */
		*ptr++ = s->prof_id[0];
		*ptr++ = s->prof_id[1];
	}
	*ptr++ = dl_param.ttl;
	*ptr++ = s->flags & PS_FLAG_MULTICAST ? 0x08 : 0x00;
	memcpy(ptr, buffer, size); ptr += size;

	return dl_send_data(s->hiface, buf, ptr - buf);
}

static uint8_t get_next_char(uint8_t **ptr)
{
	uint8_t ch;
#ifdef API_MODE2
	if (**ptr == ESC_CHAR) {
		(*ptr)++;
		**ptr ^= 0x20;
	}
#endif
	ch = **ptr;
	(*ptr)++;
	return ch;
}

/**
 * Receives a frame from physical device
 **/
int dl_recv_frame(const struct psock * s, uint8_t * buffer, int size)
{
	int val, cnt, i;
	uint16_t len;
	uint8_t *p1, *p2;
	uint16_t crc = 0;

	/* Get data from Physical device */
	val = uart_recv_buffer(s->hiface, buffer, size);
	if (val <= 0) {
		err("dl_recv_frame: phy read error\n");
		return -1;
	}

	p1 = p2 = buffer;
	cnt = val;

	/* Remove leading junk chars */
	while (*p1++ != API_HEADER && cnt--);
	/* Sanity check: We need atleast 4 bytes after header
	   to consider it as a valid packet! */
	if (cnt - 4 <= 0) {
		err("Dropping invalid packet!\n");
		return -1;
	}

	len = (get_next_char(&p1) << 8) | get_next_char(&p1);

	/* Sanity Check: Do we have sufficient data in packet? */
	if (&p1[len] > &buffer[val]) {
		err("Dropping short packet!\n");
		return -1;
	}
	for(i = 0; i < len; i++) {
		uint8_t ch = get_next_char(&p1);
		crc += ch;
		*p2++ = ch;
	}
	crc = (0xFF - crc) & 0xFF;
	if (crc != get_next_char(&p1)) {
		err("CRC Failure. Dropping packet!\n");
		return -1;
	}
	dbg("Received valid packet!\n");
	return len;
}

