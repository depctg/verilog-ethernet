/*
 * Copyright (c) 2020，Wuklab, UCSD.
 */

#include "tx_64.hpp"

enum udp_enqueue_status {
	udp_enqueue_wait,
	udp_enqueue_head,
	udp_enqueue_payload
};

enum udp_dequeue_status {
	udp_dequeue_idle,
	udp_dequeue_ack,
	udp_retrans_head,
	udp_retrans_payload_1,
	udp_retrans_payload_2
};

void tx_64(stream<struct udp_info>	*tx_header,
	   stream<struct net_axis_64>	*tx_payload,
	   stream<struct udp_info>	*usr_tx_header,
	   stream<struct net_axis_64>	*usr_tx_payload,
	   stream<struct udp_info>	*ack_header,
	   stream<struct net_axis_64>	*ack_payload,
	   stream<struct bram_cmd>	*queue_rd_cmd,
	   stream<struct bram_cmd>	*queue_wr_cmd,
	   stream<struct net_axis_64>	*queue_wr_data,
	   stream<struct net_axis_64>	*queue_rd_data,
	   stream<struct udp_info>	*rt_header,
	   stream<struct net_axis_64>	*rt_payload)
{
#pragma HLS INTERFACE axis both port=tx_header
#pragma HLS DATA_PACK variable=tx_header
#pragma HLS INTERFACE axis both port=tx_payload

#pragma HLS INTERFACE axis both port=usr_tx_header
#pragma HLS DATA_PACK variable=usr_tx_header
#pragma HLS INTERFACE axis both port=usr_tx_payload

#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS PIPELINE

	static enum udp_enqueue_status enqueue_state = udp_enqueue_wait;
	static enum udp_dequeue_status dequeue_state = udp_dequeue_idle;

	// seqnum info
	static unsigned int last_ackd_seqnum = 0;
	static unsigned int last_sent_seqnum = 0;

	// header queue info
	static struct udp_info unackd_header_queue[WINDOW_SIZE];
#pragma HLS dependence variable=unackd_header_queue intra false
#pragma HLS dependence variable=unackd_header_queue inter false
#pragma HLS ARRAY_PARTITION variable=unackd_header_queue
#pragma HLS DATA_PACK variable=unackd_header_queue

	static unsigned char head = 0;
	static unsigned char rear = 0;

	// timer info
	static bool retrans = false;
	static long timer = -1;	// timer idle

	if (timer > 0) timer--;
	if (timer == 0) retrans = true;

	static unsigned int pkt_size_cnt = 0;

	// enqueue state machine
	struct udp_info send_udp_info;
	struct net_axis_64 send_pkt;
	struct bram_cmd cmd_w;

	switch (enqueue_state) {
	case udp_enqueue_wait:
		/**
		 * wait for available slot in the queue
		 */
		if (last_sent_seqnum < last_ackd_seqnum + WINDOW_SIZE) {
			enqueue_state = udp_enqueue_head;
			last_sent_seqnum++;
		}
		break;
	case udp_enqueue_head:
		if (usr_tx_header->empty()) break;
		send_udp_info = usr_tx_header->read();
		/**
		 * send udp header to tx port and unack'd queue
		 */
		unackd_header_queue[rear] = send_udp_info;
		tx_header->write(send_udp_info);
		enqueue_state = udp_enqueue_payload;
		break;
	case udp_enqueue_payload:
		/**
		 * send udp payload to tx port and unack'd queue
		 */
		if (usr_tx_payload->empty()) break;
		send_pkt = usr_tx_payload->read();
		cmd_w.index = rear;
		cmd_w.offset = pkt_size_cnt;
		queue_wr_cmd->write(cmd_w);
		queue_wr_data->write(send_pkt);
		tx_payload->write(send_pkt);
		if (send_pkt.last == 1) {
			enqueue_state = udp_enqueue_wait;
			pkt_size_cnt = 0;
			rear = (rear + 1) & WINDOW_INDEX_MSK;
			timer = TIMEOUT;
		}
		break;
	default:
		break;
	}

	// dequeue state machine
	struct net_axis_64 ack_pkt;
	struct udp_info ack_udp_info;
	ap_uint<SEQ_WIDTH> recv_seqnum;

	static unsigned char retrans_i, retrans_end;
	static unsigned int retrans_size_cnt = 0;
	struct udp_info retrans_udp_info;
	struct net_axis_64 retrans_pkt;
	struct bram_cmd cmd_r;

	switch (dequeue_state) {
	case udp_dequeue_idle:
		if (!ack_header->empty() && !ack_payload->empty()) {
			ack_udp_info = ack_header->read();
			ack_pkt = ack_payload->read();
			dequeue_state = udp_dequeue_ack;
			break;
		}
		if (retrans) {
			dequeue_state = udp_retrans_head;
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
		// TODO: what if seqnum overflows?
		last_ackd_seqnum = recv_seqnum > last_ackd_seqnum
				       ? (unsigned int)recv_seqnum
				       : last_ackd_seqnum;
		if (recv_seqnum > last_ackd_seqnum) {
			/* move head forward */
			head = (head + (recv_seqnum - last_ackd_seqnum)) &
			       WINDOW_INDEX_MSK;
			timer = TIMEOUT;
			if (ack_pkt.data(7, 0) == pkt_type_nack) {
				/**
				 * if nack received, retransmit. restart
				 * timer after finishing retransmission
				 */
				retrans = true;
				/* set retrans range */
				retrans_i = head;
				retrans_end = rear;
				timer = -1;
				dequeue_state = udp_retrans_head;
				break;
			}
		}
		dequeue_state = udp_dequeue_idle;
		break;
	case udp_retrans_head:
		if (retrans_i == retrans_end) {
			/* retransmission finish, restart timer */
			dequeue_state = udp_dequeue_idle;
			retrans = false;
			timer = TIMEOUT;
			break;
		}
		retrans_udp_info = unackd_header_queue[retrans_i];
		rt_header->write(retrans_udp_info);
		dequeue_state = udp_retrans_payload_1;
		break;
	case udp_retrans_payload_1:
		cmd_r.index = retrans_i;
		cmd_r.offset = retrans_size_cnt;
		queue_rd_cmd->write(cmd_r);
		dequeue_state = udp_retrans_payload_2;
		break;
	case udp_retrans_payload_2:
		if (queue_rd_data->empty()) break;
		retrans_pkt = queue_rd_data->read();
		rt_payload->write(retrans_pkt);
		if (retrans_pkt.last == 1) {
			dequeue_state = udp_retrans_head;
			retrans_i = (retrans_i + 1) & WINDOW_INDEX_MSK;
			retrans_size_cnt = 0;
		}
		break;
	default:
		break;
	}
}
