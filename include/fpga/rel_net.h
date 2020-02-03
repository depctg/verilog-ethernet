#ifndef _LEGO_MEM_REL_NET_H_
#define _LEGO_MEM_REL_NET_H_

#include <ap_int.h>

#define SEQ_WIDTH 32

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
