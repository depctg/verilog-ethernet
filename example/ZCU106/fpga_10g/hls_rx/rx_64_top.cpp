/*
 * Copyright (c) 2020，Wuklab, UCSD.
 */

#include <stdio.h>
#include "rx_64_top.hpp"

enum udp_recv_status {
	//udp_recv_check_valid,
	udp_recv_udp_head,
	udp_recv_lego_head,
	udp_recv_parse_stream,
	udp_recv_ack
};

enum udp_send_status {
	udp_send_udp_head,
	udp_send_ack
};

void rx_64(stream<struct udp_info> *rx_header,
	   stream<struct net_axis_64> *rx_payload,
	   stream<struct udp_info> *tx_header,
	   stream<struct net_axis_64> *tx_payload)
{
/**
 * receive from UDP module
 */
#pragma HLS INTERFACE axis both port=rx_header
#pragma HLS DATA_PACK variable=rx_header
#pragma HLS INTERFACE axis both port=rx_payload
/**
 * send to UDP module
 */
#pragma HLS INTERFACE axis both port=tx_header
#pragma HLS DATA_PACK variable=tx_header
#pragma HLS INTERFACE axis both port=tx_payload

#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS PIPELINE

	static enum udp_recv_status status = udp_recv_udp_head;
	static unsigned int expected_seqnum = 1;

	struct net_axis_64 recv_pkt, resp_pkt;
	static struct udp_info recv_udp_info, resp_udp_info;

	static bool ack = false;

	switch (status) {
	case udp_recv_udp_head:
		if (rx_header->empty())
			break;
		recv_udp_info = rx_header->read();
		resp_udp_info.dest_ip = recv_udp_info.src_ip;
		resp_udp_info.dest_port = recv_udp_info.src_port;
		resp_udp_info.src_ip = recv_udp_info.dest_ip;
		resp_udp_info.src_port = recv_udp_info.dest_port;
		resp_udp_info.length = 8;
		printf("receive header: %x:%d -> %x:%d\n",
		       recv_udp_info.src_ip.to_uint(),
		       recv_udp_info.src_port.to_uint(),
		       recv_udp_info.dest_ip.to_uint(),
		       recv_udp_info.dest_port.to_uint());
		status = udp_recv_lego_head;
		break;
	case udp_recv_lego_head:
		if (rx_payload->empty())
			break;
		recv_pkt = rx_payload->read();
		printf("receive data %llx\n", recv_pkt.data.to_uint64());
		printf("receive lego header: [type %d, seq %lld]\n",
		       recv_pkt.data(7, 0).to_uint(),
		       recv_pkt.data(55, 8).to_uint64());
		if (recv_pkt.data(55, 8) == expected_seqnum) {
			ack = true;
			/* generate response packet */
			resp_pkt.data(7, 0) = pkt_type_ack;
			resp_pkt.data(55, 8) = expected_seqnum;
			resp_pkt.last = 1;
			resp_pkt.keep = 0xff;
			expected_seqnum++;
		}
		if (recv_pkt.last == 1)
			status = udp_recv_udp_head;
		else
			status = udp_recv_parse_stream;
		break;
	case udp_recv_parse_stream:
		if (rx_payload->empty())
			break;
		recv_pkt = rx_payload->read();
		printf("receive data %llx\n", recv_pkt.data.to_uint64());
		/**
		 * do sth with received packet
		 */
		if (recv_pkt.last == 1)
			status = udp_recv_udp_head;
		break;
	default:
		break;
	}

	if (ack) {
		/* send response udp header */
		tx_header->write(resp_udp_info);
		/* send response acknowledgment */
		tx_payload->write(resp_pkt);
		printf("response data %llx\n", resp_pkt.data.to_uint64());
		ack = false;
	}
}
