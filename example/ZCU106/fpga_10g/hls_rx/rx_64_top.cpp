/*
 * Copyright (c) 2020，Wuklab, UCSD.
 */

#include <stdio.h>
#include "rx_64.hpp"

enum udp_recv_status {
	//udp_recv_check_valid,
	udp_recv_udp_head,
	udp_recv_lego_head,
	udp_recv_parse_stream
};

void rx_64(stream<struct udp_info>	*rx_header,
	   stream<struct net_axis_64>	*rx_payload,
	   stream<struct udp_info>	*rsp_header,
	   stream<struct net_axis_64>	*rsp_payload,
	   stream<struct udp_info>	*ack_header,
	   stream<struct net_axis_64>	*ack_payload,
	   stream<struct udp_info>	*usr_rx_header,
	   stream<struct net_axis_64>	*usr_rx_payload,
	   ap_uint<1>			reset_seq)
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
#pragma HLS INTERFACE axis both port=rsp_header
#pragma HLS DATA_PACK variable=rsp_header
#pragma HLS INTERFACE axis both port=rsp_payload

#pragma HLS INTERFACE axis both port=ack_header
#pragma HLS DATA_PACK variable=ack_header
#pragma HLS INTERFACE axis both port=ack_payload

#pragma HLS INTERFACE axis both port=usr_rx_header
#pragma HLS DATA_PACK variable=usr_rx_header
#pragma HLS INTERFACE axis both port=usr_rx_payload

#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS PIPELINE

	static enum udp_recv_status status = udp_recv_udp_head;
	static unsigned int expected_seqnum = 1;

	struct net_axis_64 recv_pkt, resp_pkt;
	static struct udp_info recv_udp_info, resp_udp_info;

	static bool ack = false;
	static bool nack_enable = true;
	static bool fetch_data = true;

	/**
	 * * This is for test use, may be removed during synthesis
	 */
	if (reset_seq) {
		expected_seqnum = 1;
		status = udp_recv_udp_head;
		ack = false;
		nack_enable = true;
	}

	switch (status) {
	case udp_recv_udp_head:
		if (rx_header->empty())
			break;
		recv_udp_info = rx_header->read();
		/* generate response header */
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
		printf("receive lego header: [type %d, seq %lld]\n",
		       recv_pkt.data(7, 0).to_uint(),
		       recv_pkt.data(7 + SEQ_WIDTH, 8).to_uint64());

		if (recv_pkt.data(7, 0) == pkt_type_data) {
			/**
			 * if received data packet, generate ack/nack response
			 */
			if (recv_pkt.data(7 + SEQ_WIDTH, 8) == expected_seqnum) {
				ack = true;
				nack_enable = true;
				fetch_data = true;
				/* generate response packet */
				resp_pkt.data(7, 0) = pkt_type_ack;
				resp_pkt.data(7+SEQ_WIDTH, 8) = expected_seqnum;
				resp_pkt.last = 1;
				resp_pkt.keep = 0xff;
				expected_seqnum++;
			} else if (nack_enable && (recv_pkt.data(7 + SEQ_WIDTH, 8) >
						expected_seqnum)) {
				ack = true;
				nack_enable = false;
				fetch_data = false;
				resp_pkt.data(7, 0) = pkt_type_nack;
				resp_pkt.data(7 + SEQ_WIDTH, 8) = expected_seqnum;
				resp_pkt.last = 1;
				resp_pkt.keep = 0xff;
			}
		}
		else {
			/**
			 * if received ack/nack, forward it to Uack'd packets queue
			 */
			ack_header->write(resp_udp_info);
			ack_payload->write(recv_pkt);
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
		if (fetch_data) {
			/**
			 * send data packet to onboard pipeline
			 * ?: what's the interface for onboard pipeline?
			 */
			usr_rx_payload->write(recv_pkt);
			usr_rx_header->write(recv_udp_info);
		}
		if (recv_pkt.last == 1) {
			status = udp_recv_udp_head;
			fetch_data = false;
		}
		break;
	default:
		break;
	}

	if (ack) {
		/* send response udp header */
		rsp_header->write(resp_udp_info);
		/* send response acknowledgment */
		rsp_payload->write(resp_pkt);
		printf("response data %llx\n", resp_pkt.data.to_uint64());
		ack = false;
	}
}
