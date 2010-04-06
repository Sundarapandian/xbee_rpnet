#include <mysql.h>
#include <xbee/api.h>

static struct rx_pkt rx;
const char * const str_mstat[] = {
	"MODEM_STAT_HW_RST",
	"MODEM_STAT_WDT_RST",
	"MODEM_STAT_ASSOC",
	"MODEM_STAT_DISASSOC",
	"MODEM_STAT_SYNC_LOST",
	"MODEM_STAT_COORD_REALIGN",
	"MODEM_STAT_COORD_START",
};

const char * const str_atstat[] = {
	"AT_OK",
	"AT_ERROR",
	"AT_CMD_INV",
	"AT_PARAM_INV",
};

int at_response_handler(struct at_response * res)
{
		switch(ATVAL(res->cmd[0], res->cmd[1])) {
			case ATVAL('A', 'O'):
				dl_param.mode_ao = res->data[0];
				dbg("Setting AO Mode to %d\n", res->data[0]);
				break;

			case ATVAL('A', 'P'):
				dl_param.mode_ap = res->data[0];
				dbg("Setting AP Mode to %d\n", res->data[0]);
				break;
		}
		return 0;
}

MYSQL * db_connect(void)
{
	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;

	char *server = "localhost";
	char *user = "sundar";
	char *password = "test"; /* set me first */
	char *database = "rpnet";

	conn = mysql_init(NULL);

	/* Connect to database */
	if (!mysql_real_connect(conn, server,
		 user, password, database, 0, NULL, 0)) {
	  fprintf(stderr, "%s\n", mysql_error(conn));
	  return NULL;
	}
	return conn;
}

int handle_data(const struct rx_data0 * rx)
{
	static MYSQL *conn;
	static char query[500];
	const char * buffer = (const char *)rx->data;
	if (conn == NULL) {
		if ((conn = db_connect()) < 0) {
			err ("Unable to connect database!\n");
			return -1;
		}
		dbg("Database Connection successful!\n");
	}
	if (*buffer++ == '<') {
		snprintf(query, sizeof(query) -1, "insert into userdata values("
			"\'%02X%02X%02X%02X%02X%02X%02X%02X\', \'%d\', NOW())",
			rx->addr64[0],rx->addr64[1],rx->addr64[2], rx->addr64[3],
			rx->addr64[4],rx->addr64[5],rx->addr64[6], rx->addr64[7],
			*(uint32_t *) buffer
			);
		printf ("%s", query);
	}
	return 0;
}

int rx_handler(const struct psock * ps)
{
	int val;
	struct at_response *res;
	struct at_remote_response *rres;
	struct rx_data0 *rx0;

	val = dl_recv_frame(ps, &rx);
	if (val <= 0) return 0; /* Let caller deal with err */

	dbg ("Rx_Thread: Received %d bytes from uart!\n", val);
	switch (rx.id) {
		case API_ID_MODEM_STATUS:
			dbg("Modem status received with value %s\n", str_mstat[rx.pkt.mstat.status]);
			break;

		case API_ID_ATREPLY:
			res = &rx.pkt.atr;
			dbg("AT Response received for cmd %2s from Local device\n", res->cmd);
			dbg("STATUS: %s\n", str_atstat[res->status]);
			if (res->status == AT_OK)
					at_response_handler(res);
			break;
		case API_ID_REMOTE_ATREPLY:
			rres = &rx.pkt.ratr;
			dbg("AT response for remote cmd %2s\n", rres->cmd);
			break;

		case API_ID_RX:
			rx0 = &rx.pkt.rx0;
			handle_data(rx0);
			break;
		default:
			dbg("Support for packet id %#02x is unimplemented\n", rx.id);
	};
#if 0
	for (i = 0; i < val; i++) {
		printf ("%02X ", buf[i]);
	}
	printf ("\n");
#endif

	return val;
}

