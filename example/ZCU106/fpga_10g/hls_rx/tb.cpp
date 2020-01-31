#include <stdio.h>
#include "rx_64_top.hpp"
#define MAX_CYCLE 20

int cycle = 0;

class test_util {
       public:
	test_util();
	~test_util() {}
	void run_one_cycle(stream<struct udp_info> *rx_header,
			   stream<struct net_axis_64> *rx_payload);

       private:
	stream<struct udp_info> tx_header;
	stream<struct net_axis_64> tx_payload;
};

test_util::test_util()
    : tx_header("send udp header info"), tx_payload("send udp packet") {
	cycle = 0;
}

void test_util::run_one_cycle(stream<struct udp_info> *rx_header,
			      stream<struct net_axis_64> *rx_payload) {
	struct udp_info recv_hd;
	struct net_axis_64 recv_data;

	rx_64(rx_header, rx_payload, &tx_header, &tx_payload);

	if (!tx_header.empty()) {
		recv_hd = tx_header.read();
		printf("[cycle %2d] host receive %x:%d -> %x:%d\n", cycle,
		       recv_hd.src_ip.to_uint(), recv_hd.src_port.to_uint(),
		       recv_hd.dest_ip.to_uint(), recv_hd.dest_port.to_uint());
	}
	if (!tx_payload.empty()) {
		recv_data = tx_payload.read();
		printf("[cycle %2d] host recv data %llx, ", cycle,
		       recv_data.data.to_uint64());
		ap_uint<8> type = recv_data.data(7, 0);
		ap_uint<48> seqnum = recv_data.data(55, 8);
		printf("if lego header: [type %d, seq %lld]\n", type.to_ushort(), seqnum.to_uint64());
	}

	cycle++;
}

struct net_axis_64 build_lego_header(ap_uint<8> type, ap_uint<48> seqnum) {
	struct net_axis_64 pkt;
	pkt.data(7, 0) = type;
	pkt.data(55, 8) = seqnum;
	pkt.keep = 0xff;
	pkt.last = 1;
	pkt.user = 0;
	return pkt;
}

int main() {
	test_util rx_64_util;
	struct udp_info test_header;
	struct net_axis_64 test_payload;
	stream<struct udp_info> rx_header("receive udp header info");
	stream<struct net_axis_64> rx_payload("receive udp packet");

	test_header.src_ip = 0xc0a80101;   // 192.168.1.1
	test_header.dest_ip = 0xc0a80180;  // 192.168.1.128
	test_header.src_port = 1234;
	test_header.dest_port = 2345;
	test_payload = build_lego_header(pkt_type_data, 1);
	printf("[cycle %2d] host send %x:%d -> %x:%d\n", cycle,
	       test_header.src_ip.to_uint(),
	       test_header.src_port.to_uint(),
	       test_header.dest_ip.to_uint(),
	       test_header.dest_port.to_uint());
	rx_header.write(test_header);
	printf("[cycle %2d] host send data %llx\n", cycle,
	       test_payload.data.to_uint64());
	rx_payload.write(test_payload);
	
	rx_64_util.run_one_cycle(&rx_header, &rx_payload);

	for (; cycle < MAX_CYCLE;) {
		rx_64_util.run_one_cycle(&rx_header, &rx_payload);
	}
	return 0;
}
