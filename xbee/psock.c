#include <string.h>

#include "xbee/psock.h"

/**
 * Array of available interfaces. They are exported from
 * interface specific files like uart.c!
 **/
extern void *h_iface[]; /* Handle to interfaces */
extern int nr_iface; /* Total number of interfaces */

struct psock * psock_local(int iface, struct psock * s, uint8_t flags)
{
	memset(s, 0, sizeof *s);
	if (iface >= nr_iface) {
		err("psock_local: Invalid interface number %d\n", iface);
		return NULL;
	}
	s->hiface = h_iface[iface];
	s->flags = flags | PS_FLAG_LOCAL;
	s->valid = PS_VALID;
	return s;
}

void psock_set_addr64(struct psock * s, uint32_t addr_hi, uint32_t addr_lo)
{
	/* Assign the 64 bit address */
	s->addr64[0] = (addr_hi >> 24) & 0xFF;
	s->addr64[1] =  (addr_hi >> 16) & 0xFF;
	s->addr64[2] =  (addr_hi >> 8) & 0xFF;
	s->addr64[3] =  addr_hi & 0xFF;
	s->addr64[4] = (addr_lo >> 24) & 0xFF;
	s->addr64[5] =  (addr_lo >> 16) & 0xFF;
	s->addr64[6] =  (addr_lo >> 8) & 0xFF;
	s->addr64[7] =  addr_lo & 0xFF;
}

void psock_set_addr16(struct psock * s, uint16_t addr)
{
	/* Assign 16-bit address */
	s->addr16[0] = (addr >> 8) & 0xFF;
	s->addr16[1] = addr & 0xFF;
}

struct psock * psock_init(
		struct psock *s, /* Pointer to pseudo socket */
		int iface, /* Interface ID */
		uint32_t addr64_hi, /* 64-bit Destination address HI */
		uint32_t addr64_lo, /* 64-bit Destination address LOW */
		uint16_t addr16, /* 16-bit Destination address */
		uint8_t sep, /* Source end point */
		uint8_t dep, /* Destination end point */
		uint8_t cid, /* Cluster ID, if = 0 simple tx frame will be sent */
		uint16_t prof_id, /* Profile ID currently only PSOCK_PROFILE_DEFAULT is supported */
		uint8_t flags) /* Check combination of PS_FLAG_XXXX */
{
	if (iface >= nr_iface) {
		err("psock_init: Invalid interface number %d\n", iface);
		return NULL;
	}

	if (dep == PS_EP_DATA &&
		(cid != PS_CID_SERIALDATA && cid != PS_CID_LOOPBACK &&
		 cid != PS_CID_IOSAMPLE && cid != PS_CID_SENSORSAMPLE &&
		 cid != PS_CID_NODEID)) {
		err("Invalid Cluster ID for endpoint %02x\n", dep);
		return NULL;
	}

	memset(s, 0, sizeof *s);
	s->hiface = h_iface[iface];

	/* Set 64 bit address */
	psock_set_addr64(s, addr64_hi, addr64_lo);

	/* Assign 16-bit address */
	psock_set_addr16(s, addr16);

	/* Assign profile ID */
	s->prof_id[0] = (prof_id >> 8) & 0xFF;
	s->prof_id[1] = prof_id & 0xFF;

	/* Assign other parameters */
	psock_set_sendpoint(s, sep);
	psock_set_dendpoint(s, dep);
	s->cid = cid;
	s->flags = flags | PS_FLAG_REMOTE;
	s->valid = PS_VALID;
	return s;
}
