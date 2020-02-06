#ifndef _RELNET_TX64_H_
#define _RELNET_TX64_H_

#include <fpga/rel_net.h>
#include <fpga/axis_net.h>
#include <hls_stream.h>
#include <bits/stdint-uintn.h>

using hls::stream;

struct bram_cmd {
	uint8_t			index;
	uint16_t		offset;
};

/**
 * @tx_header: udp header sent to network stack
 * @tx_payload: udp payload sent to network stack
 * @usr_tx_header: received udp header from onboard pipeline
 * @usr_tx_payload: received udp payload from onboard pipeline
 * @unack_header: udp header sent to unack'd queue
 * @unack_payload: udp payload sent to unack's queue
 */
void tx_64(stream<struct udp_info>	*tx_header,
	   stream<struct net_axis_64>	*tx_payload,
	   stream<struct udp_info>	*usr_tx_header,
	   stream<struct net_axis_64>	*usr_tx_payload,
	   stream<struct bram_cmd>	*queue_rd_cmd,
	   stream<struct bram_cmd>	*queue_wr_cmd,
	   stream<struct net_axis_64>	*queue_rd_data,
	   stream<struct net_axis_64>	*queue_wr_data);

#endif