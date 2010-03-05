#ifndef _PSOCK_H
#define _PSOCK_H

#include "arch/common.h"

/**
 * List of endpoints supported by Xbee
 **/
#define PS_EP_DATAOBJECT      0x00
#define PS_EP_USERBASE        0x01
#define PS_EP_COMMAND         0xE6
#define PS_EP_DATA            0xE8

/**
 * List of clustre IDs supported for PS_EP_DATA
 **/
#define PS_CID_SERIALDATA     0x11
#define PS_CID_LOOPBACK       0x12
#define PS_CID_IOSAMPLE       0x92
#define PS_CID_SENSORSAMPLE   0x94
#define PS_CID_NODEID         0x95

/**
 * PS_FLAG_MULTICAST --> Selects Multicast/Unicast transfer mode
 * PS_FLAG_REMOTE    --> Internal: Identifies a remote socket
 * PS_FLAG_DISCARD_REPLY --> Discards reponse from remote (Used in AT command)
 * PS_FLAG_APPLY_CHANGE --> Apply changes done to the remote (Used in Remote AT command)
 * PS_FLAG_LOCAL --> Internal: Indicates that the socket acts on local device
 **/
#define PS_FLAG_MULTICAST        _BIT(1)
#define PS_FLAG_REMOTE           _BIT(2)
#define PS_FLAG_DISCARD_REPLY    _BIT(3)
#define PS_FLAG_QUEUE_CHANGE     _BIT(4)
#define PS_FLAG_LOCAL            _BIT(7)

/* Only supported profile as of now */
#define PSOCK_PROFILE_DEFAULT    0xC105

/* Broadcast 64-Bit address */
#define ADDR64_BROADCAST_HI      0x0UL
#define ADDR64_BROADCAST_LO      0xFFFFUL

/* Co-ordinator address (Default) */
#define ADDR64_COORDINATOR_HI    0x0UL
#define ADDR64_COORDINATOR_LO    0x0UL

/* 16-bit address for any device */
#define ADDR16_ANY               0xFFFE

/* Magic number for valid socket */
#define PS_VALID                 0x5A

#define is_ps_valid(ps) ((ps)->valid == PS_VALID)
#define psock_set_sendpoint(ps,ep) ((ps)->sep = (ep))
#define psock_set_dendpoint(ps,ep) ((ps)->dep = (ep))

/**
 * Psuedo-Socket datastructure
 **/
struct  psock {
	void * hiface;      /* Handle to the interface */
	uint8_t addr64[8];  /* 64-bit Dest addr */
	uint8_t addr16[2];  /* 16-bit Dest addr */
	uint8_t sep;        /* Source End-Point */
	uint8_t dep;        /* Destination End-Point */
	uint8_t cid;        /* Cluster ID */
	uint8_t prof_id[2]; /* Profile ID */
	uint8_t nid[20];    /* ASCII Node ID */
	uint8_t flags;      /* For available flags check PS_FLAG_XXXXX */
	uint8_t valid;      /* Used to check if it is valid socket */
};

#ifdef __cplusplus
extern "C" {
#endif
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
		uint8_t flags); /* Check combination of PS_FLAG_XXXX */

struct psock * psock_local(int iface, struct psock * s, uint8_t flags);

#ifdef __cplusplus
}
#endif
#endif /* ndef _PSOCK_H */
