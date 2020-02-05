#ifndef _LEGO_MEM_REL_NET_H_
#define _LEGO_MEM_REL_NET_H_

#include <ap_int.h>

#define SEQ_WIDTH	32
/**
 * ? what's the max packet size?
 * a reasonable guess: pagesize(4K) + lego_header(8 byte)
 * every transfer is 64 bit, thus ÷8
 */
#define MAX_PACKET_SIZE		4104 / 8
#define WINDOW_INDEX_MSK	0x07
#define WINDOW_SIZE		8

#define TIMEOUT			10000000

enum pkt_type {
	pkt_type_data,
	pkt_type_ack,
	pkt_type_nack
};

struct lego_header {
	ap_uint<8>		type;
	ap_uint<SEQ_WIDTH>	seqnum;
};

#endif