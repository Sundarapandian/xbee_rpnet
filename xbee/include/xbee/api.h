#ifndef _DL_API_H
#define _DL_API_H

#include "xbee/psock.h"

#define API_HEADER 0x7E
#define ESC_CHAR 0x7D
#define BUF_SIZE 80

/**
 * List of XBee API commands
 * Protocol ZNET 2.5
 **/
#define API_ID_MODEM_STATUS   0x8A
#define API_ID_ATCMD          0x08
#define API_ID_ATCMD_QPV      0x09 /* AT Command - Queue Parameter Value */
#define API_ID_ATREPLY        0x88 /* AT Command Response */
#define API_ID_REMOTE_ATCMD   0x17 /* Remote Command Request */
#define API_ID_REMOTE_ATREPLY 0x97 /* Remote Command Response */
#define API_ID_TX             0x10 /* ZigBee Transmit Request */
#define API_ID_TX_EXPLICIT    0x11 /* Explicit Addressing Tx Frame */
#define API_ID_TX_STATUS      0x8B /* ZigBee Transmit Status */
#define API_ID_RX             0x90 /* ZigBee Receive Packet (AO=0) */
#define API_ID_RX_EXPLICIT    0x91 /* ZigBee Explicit Rx Indicator (AO=1) */
#define API_ID_RX_DATA_SAMPLE 0x92 /* ZigBee IO Data Sample Rx Indicator */
#define API_ID_SENSOR_RD      0x94 /* XBee Sensor Read Indicator (AO=0) */
#define API_ID_NODEID         0x95 /* Node Identification Indicator (AO=0) */

/* Possible API modes */
#define API_MODE1             0x01
#define API_MODE2             0x02

/**
 * Default mode of operation
 * During device enumeration time these values
 * will be indentified during runtime.
 * The below values will be used only when there is
 * some serious problem in device enumeration.
 **/
#define DL_PARAM_TTL_DEFAULT        0x00
#define DL_PARAM_APIMODE_DEFAULT    API_MODE2
#define DL_PARAM_AOMODE_DEFAULT     0x00

/**
 * Return true if x is FRAME_HEADER (0x7E) or ESC_CHAR (0x7D)
 * or XON (0x11) or XOFF (0x13). Else return false
 **/
#define is_esc_char(x) ((x == 0x7E) || (x == 0x7D) \
		|| (x == 0x11) || (x == 0x13))

/* Sets a DL-Layer parameter */
#define dl_set_param(mem, val) (dl_param.(mem) = val)

struct dl_param {
	uint8_t ttl;
	uint8_t mode_ap;
	uint8_t mode_ao;
};

extern struct dl_param dl_param;

enum modem_stat {
	MODEM_STAT_HW_RST = 0,
	MODEM_STAT_WDT_RST,
	MODEM_STAT_ASSOC,
	MODEM_STAT_DISASSOC,
	MODEM_STAT_SYNC_LOST,
	MODEM_STAT_COORD_REALIGN,
	MODEM_STAT_COORD_START,
};

struct modem_status {
	uint8_t status;
}__attribute__((packed));

enum at_response_status {
	AT_OK = 0,
	AT_ERROR,
	AT_CMD_INV,
	AT_PARAM_INV,
};

struct at_response {
	uint8_t frame_id;
	uint8_t cmd[2];
	uint8_t status;
	uint8_t data[BUF_SIZE];
}__attribute__((packed));

struct at_remote_response {
	uint8_t frame_id;
	uint8_t addr64[8];
	uint8_t addr16[2];
	uint8_t cmd[2];
	uint8_t status;
	uint8_t data[BUF_SIZE];
}__attribute__((packed));

enum delivery_status {
	TX_STAT_OK = 0,
	TX_STAT_ERR_CCA,
	TX_STAT_ERR_EP,
	TX_STAT_ERR_ACK,
	TX_STAT_ERR_NONET,
	TX_STAT_ERR_SELF,
	TX_STAT_ERR_NOADDR,
	TX_STAT_ERR_NOROUTE,
};

enum discovery_status {
	DIS_STAT_OK = 0,
	DIS_ADDR,
	DIS_ROUTE,
	DIS_ROUTEADDR,
};

struct tx_status {
	uint8_t frame_id;
	uint8_t addr16[2];
	uint8_t retry_cnt;
	uint8_t dlvry_stat;
	uint8_t disc_stat;
};

struct rx_data0 {
	uint8_t addr64[8];
	uint8_t addr8[2];
	uint8_t type;
	uint8_t data[BUF_SIZE];
};

struct rx_pkt {
	uint8_t id;
	union {
		struct modem_status mstat;
		struct tx_status txstat;
		struct at_response atr;
		struct at_remote_response ratr;
		struct rx_data0  rx0;
		uint8_t raw[BUF_SIZE];
	}pkt;
};

#define ATVAL(x,y) (((x) << 8) | (y))
#ifdef __cplusplus
extern "C" {
#endif
	int dl_send_at_command(const struct psock * s, const uint8_t * cmd);
	int dl_recv_frame(const struct psock * s, struct rx_pkt * rx);
	int dl_tx_data(const struct psock * s, const uint8_t * buff, int size);
	void dl_set_param_ttl(uint8_t ttl);
#ifdef __cplusplus
}
#endif

#endif /* ndef _DL_API_H */
