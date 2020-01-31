#ifndef _RELNET_RX64_H_
#define _RELNET_RX64_H_

#include <ap_int.h>
#include <hls_stream.h>
#include <fpga/axis_net.h>

using hls::stream;

/**
 * leave the udp header unprocessed, only care about the payload
 */
void rx_64(stream<struct udp_info> *rx_header,
	   stream<struct net_axis_64> *rx_payload,
	   stream<struct udp_info> *tx_header,
	   stream<struct net_axis_64> *tx_payload);

enum pkt_type {
	pkt_type_data,
	pkt_type_ack
};

struct lego_header {
	ap_uint<8>	type;
	ap_uint<48>	seqnum;
};

#endif
