/*
 * Copyright (c) 2020，Wuklab, UCSD.
 */

#include "tx_64.hpp"

enum udp_send_status {
	udp_send_wait,
	udp_send_udp_head,
	udp_send_payload
};

void tx_64_sender(stream<struct udp_info>	*tx_header,
		  stream<struct net_axis_64>	*tx_payload,
		  stream<struct udp_info>	*usr_tx_header,
		  stream<struct net_axis_64>	*usr_tx_payload,
		  stream<struct udp_info>	*unack_header,
		  stream<struct net_axis_64>	*unack_payload,
		  unsigned int last_ackd_seqnum)
{
#pragma HLS INTERFACE axis both port=tx_header
#pragma HLS DATA_PACK variable=tx_header
#pragma HLS INTERFACE axis both port=tx_payload

#pragma HLS INTERFACE axis both port=usr_tx_header
#pragma HLS DATA_PACK variable=usr_tx_header
#pragma HLS INTERFACE axis both port=usr_tx_payload

#pragma HLS INTERFACE axis both port=unack_header
#pragma HLS DATA_PACK variable=unack_header
#pragma HLS INTERFACE axis both port=unack_payload

#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS PIPELINE

	static enum udp_send_status state = udp_send_wait;

	static unsigned int last_sent_seqnum = 0;

	struct udp_info send_udp_info;
	struct net_axis_64 send_pkt;

	switch (state) {
	case udp_send_wait:
		/**
		 * wait for available slot in the queue
		 */
		if (last_sent_seqnum < last_ackd_seqnum + WINDOW_SIZE) {
			state = udp_send_udp_head;
			last_sent_seqnum++;
		}
		break;
	case udp_send_udp_head:
		if (usr_tx_header->empty()) break;
		send_udp_info = usr_tx_header->read();
		/**
		 * send udp header to tx port and unack'd queue
		 */
		unack_header->write(send_udp_info);
		tx_header->write(send_udp_info);
		state = udp_send_payload;
		break;
	case udp_send_payload:
		/**
		 * send udp payload to tx port and unack'd queue
		 */
		if (usr_tx_payload->empty()) break;
		send_pkt = usr_tx_payload->read();
		unack_payload->write(send_pkt);
		tx_payload->write(send_pkt);
		if (send_pkt.last == 1) state = udp_send_wait;
		break;
	default:
		break;
	}
}
