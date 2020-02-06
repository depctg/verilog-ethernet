/*
 * Copyright (c) 2020，Wuklab, UCSD.
 */

#include <stdio.h>
#include "queue_64.hpp"

static ap_uint<SEQ_WIDTH> last_ackd_seqnum;

enum udp_enqueue_status {
	udp_enqueue_head,
	udp_enqueue_payload
};

enum udp_dequeue_status {
	udp_dequeue_idle,
	udp_dequeue_ack,
	udp_retrans_head,
	udp_retrans_payload,
	udp_retrans_check
};

void queue_64(stream<struct udp_info>		*ack_header,
	      stream<struct net_axis_64>	*ack_payload,
	      stream<struct udp_info>		*unack_header,
	      stream<struct net_axis_64>	*unack_payload,
	      stream<struct udp_info>		*tx_header,
	      stream<struct net_axis_64>	*tx_payload)
	      //ap_uint<SEQ_WIDTH>		*last_ackd_seqnum)
{
#pragma HLS DATA_PACK variable=ack_header
#pragma HLS DATA_PACK variable=ack_payload
#pragma HLS DATA_PACK variable=unack_header
#pragma HLS DATA_PACK variable=unack_payload

#pragma HLS INTERFACE axis both port=tx_header
#pragma HLS DATA_PACK variable=tx_header
#pragma HLS INTERFACE axis both port=tx_payload

#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS PIPELINE

/**
 * ! here we assume there is no intra or inter cycle dependency
 * ! since I always enqueue from the rear while dequeue from the head
 * ! but I can not be very sure and further test need to be done
 */
	static struct net_axis_64 unackd_payload_queue[WINDOW_SIZE][MAX_PACKET_SIZE];
//#pragma HLS RESOURCE variable=unackd_payload_queue core=RAM_2P_LUTRAM
#pragma HLS dependence variable=unackd_payload_queue intra false
#pragma HLS dependence variable=unackd_payload_queue inter false
#pragma HLS ARRAY_PARTITION variable=unackd_payload_queue dim=1
#pragma HLS DATA_PACK variable=unackd_payload_queue
	static struct udp_info unackd_header_queue[WINDOW_SIZE];
#pragma HLS dependence variable=unackd_header_queue intra false
#pragma HLS dependence variable=unackd_header_queue inter false
#pragma HLS ARRAY_PARTITION variable=unackd_header_queue
#pragma HLS DATA_PACK variable=unackd_header_queue

	static unsigned char head = 0;
	static unsigned char rear = 0;

	static unsigned int pkt_size_cnt = 0;

	static enum udp_enqueue_status enqueue_state = udp_enqueue_head;
	static enum udp_dequeue_status retrans_state = udp_dequeue_idle;

	struct udp_info unackd_udp_info;
	struct net_axis_64 unackd_pkt;

	static bool retrans = false;
	static long timer = -1;	// timer idle

	if (timer > 0) timer--;
	if (timer == 0) retrans = true;

	/* add unacked packets to queue */
	switch (enqueue_state) {
	case udp_enqueue_head:
		/* add udp header */
		if (unack_header->empty()) break;
		unackd_udp_info = unack_header->read();
		unackd_header_queue[rear] = unackd_udp_info;
		enqueue_state = udp_enqueue_payload;
		break;
	case udp_enqueue_payload:
		/**
		 * add udp payload, and start timer after
		 * adding a complete packet to queue
		 */
		if (unack_payload->empty()) break;
		unackd_pkt = unack_payload->read();
		unackd_payload_queue[rear][pkt_size_cnt] = unackd_pkt;
		pkt_size_cnt++;
		if (unackd_pkt.last == 1) {
			pkt_size_cnt = 0;
			enqueue_state = udp_enqueue_head;
			rear = (rear + 1) & WINDOW_INDEX_MSK;
			timer = TIMEOUT;
		}
		break;
	default:
		break;
	}

	struct net_axis_64 ack_pkt;
	struct udp_info ack_udp_info;
	ap_uint<SEQ_WIDTH> recv_seqnum;
	unsigned char i;

	static unsigned char retrans_i, retrans_end;
	static unsigned int retrans_size_cnt = 0;
	struct udp_info retrans_udp_info;
	static struct net_axis_64 retrans_pkt;

	switch (retrans_state) {
	case udp_dequeue_idle:
		if (!ack_header->empty() && !ack_payload->empty()) {
			ack_udp_info = ack_header->read();
			ack_pkt = ack_payload->read();
			retrans_state = udp_dequeue_ack;
			break;
		}
		if (retrans) {
			retrans_state = udp_retrans_head;
			retrans_i = head;
			retrans_end = rear;
			break;
		}
		break;
	case udp_dequeue_ack:
		/**
		 * remove acked packets from unacked packets queue (ack)
		 * or retransmit unacked packets (nack)
		 */
		recv_seqnum = ack_pkt.data(7 + SEQ_WIDTH, 8);
		last_ackd_seqnum = recv_seqnum > last_ackd_seqnum ? recv_seqnum : last_ackd_seqnum;
		if (recv_seqnum > last_ackd_seqnum) {
			/* find the packet to acknowledge */
			for (i = 0; i < WINDOW_SIZE; i++) {
				if (unackd_payload_queue[i][0].data(
					7 + SEQ_WIDTH, 8) == recv_seqnum) {
					break;
				}
			}
			head = (i + 1) & WINDOW_INDEX_MSK;
			timer = TIMEOUT;
			if (ack_pkt.data(7, 0) == pkt_type_nack) {
				/**
				 * if nack received, retransmit. restart
				 * timer after finishing retransmission
				 */
				retrans = true;
				timer = -1;
			}
		}
		if (retrans) {
			/* set retrans range */
			retrans_state = udp_retrans_head;
			retrans_i = head;
			retrans_end = rear;
		} else
			retrans_state = udp_dequeue_idle;
		break;
	case udp_retrans_head:
		if (retrans_i == retrans_end) {
			/* retransmission finish, restart timer */
			retrans_state = udp_dequeue_idle;
			retrans = false;
			timer = TIMEOUT;
			break;
		}
		retrans_udp_info = unackd_header_queue[retrans_i];
		retrans_pkt =
			unackd_payload_queue[retrans_i][retrans_size_cnt];
		tx_header->write(retrans_udp_info);
		retrans_state = udp_retrans_payload;
		break;
	case udp_retrans_payload:
		retrans_size_cnt++;
		if (retrans_pkt.last == 1) {
			/* retrans the next packet in the queue */
			tx_payload->write(retrans_pkt);
			retrans_state = udp_retrans_head;
			retrans_i = (retrans_i + 1) & WINDOW_INDEX_MSK;
			retrans_size_cnt = 0;
			break;
		}
		tx_payload->write(retrans_pkt);
		retrans_pkt =
			unackd_payload_queue[retrans_i][retrans_size_cnt];
		break;
	default:
		break;
	}
}
