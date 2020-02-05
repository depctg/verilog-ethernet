#ifndef _RELNET_QUEUE64_H_
#define _RELNET_QUEUE64_H_

#include <fpga/rel_net.h>
#include <fpga/axis_net.h>
#include <hls_stream.h>

using hls::stream;

struct queue_cmd {};

/**
 * @ack_header: udp header of ack/nack from receiver(may not need)
 * @ack_payload: udp payload of ack/nack from receiver
 * @unack_header: udp header of unacked packet from sender
 * @unack_payload: udp payload of unacked packet from sender
 * @enq_header: udp header to enqueue
 * @enq_payload: udp payload to enqueue
 * @cmd: enqueue/dequeue command sent to queue
 */
void queue_64(stream<struct udp_info>		*ack_header,
	      stream<struct net_axis_64>	*ack_payload,
	      stream<struct udp_info>		*unack_header,
	      stream<struct net_axis_64>	*unack_payload,
	      stream<struct udp_info>		*enq_header,
	      stream<struct net_axis_64>	*enq_payload,
	      stream<struct queue_cmd>		*cmd,
	      volatile unsigned int		*last_ackd_seqnum);

/**
 * @enq_header: udp header to enqueue
 * @enq_payload: udp payload to enqueue
 * @tx_header: udp header sent to network stack
 * @tx_payload: udp payload sent to network stack
 * @cmd: enqueue/dequeue command
 */
void unacked_queue(stream<struct udp_info>	*enq_header,
		   stream<struct net_axis_64>	*enq_payload,
		   stream<struct udp_info>	*tx_header,
		   stream<struct net_axis_64>	*tx_payload,
		   stream<struct queue_cmd>	*cmd);

#endif
