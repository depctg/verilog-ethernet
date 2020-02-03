/*
 * Copyright (c) 2020，Wuklab, UCSD.
 */

`timescale 1ns / 1ps

module rx_64_wrapper (
	clk,
	rst,
	// UDP frame input
	s_udp_hdr_valid,
	s_udp_hdr_ready,
	s_udp_src_ip,
	s_udp_dest_ip,
	s_udp_src_port,
	s_udp_dest_port,
	s_udp_length,
	s_udp_payload_axis_tdata,
	s_udp_payload_axis_tvalid,
	s_udp_payload_axis_tready,
	s_udp_payload_axis_tlast,
	s_udp_payload_axis_tkeep,
	s_udp_payload_axis_tuser,
	// UDP frame output
	m_udp_hdr_valid,
	m_udp_hdr_ready,
	/*
	m_udp_ip_desp,
	m_udp_ip_ecn,
	m_udp_ip_ttl,
	*/
	m_udp_src_ip,
	m_udp_dest_ip,
	m_udp_src_port,
	m_udp_dest_port,
	m_udp_length,
	m_udp_payload_axis_tdata,
	m_udp_payload_axis_tvalid,
	m_udp_payload_axis_tready,
	m_udp_payload_axis_tlast,
	m_udp_payload_axis_tkeep,
	m_udp_payload_axis_tuser
);

input  wire clk;
input  wire rst;
// UDP frame input
input  wire s_udp_hdr_valid;
output wire s_udp_hdr_ready;
input  wire [31:0] s_udp_src_ip;
input  wire [31:0] s_udp_dest_ip;
input  wire [15:0] s_udp_src_port;
input  wire [15:0] s_udp_dest_port;
input  wire [15:0] s_udp_length;
input  wire [63:0] s_udp_payload_axis_tdata;
input  wire s_udp_payload_axis_tvalid;
output wire s_udp_payload_axis_tready;
input  wire s_udp_payload_axis_tlast;
input  wire [7:0] s_udp_payload_axis_tkeep;
input  wire s_udp_payload_axis_tuser;
// UDP frame output
output wire m_udp_hdr_valid;
input  wire m_udp_hdr_ready;
/*
output wire [5:0] m_udp_ip_desp;
output wire [1:0] m_udp_ip_ecn;
output wire [7:0] m_udp_ip_ttl;
*/
output wire [31:0] m_udp_src_ip;
output wire [31:0] m_udp_dest_ip;
output wire [15:0] m_udp_src_port;
output wire [15:0] m_udp_dest_port;
output wire [15:0] m_udp_length;
output wire [63:0] m_udp_payload_axis_tdata;
output wire m_udp_payload_axis_tvalid;
input  wire m_udp_payload_axis_tready;
output wire m_udp_payload_axis_tlast;
output wire [7:0] m_udp_payload_axis_tkeep;
output wire m_udp_payload_axis_tuser;

wire [111:0] rx_udp_hdr_info;
wire [111:0] tx_udp_hdr_info;

/*
 * bundle input UDP header info
 * udp length at highest position,
 * src ip at lowest position
 */
assign rx_udp_hdr_info = {
	s_udp_length,
	s_udp_dest_port,
	s_udp_src_port,
	s_udp_dest_ip,
	s_udp_src_ip
};
// bundle output UDP header info
assign {
	m_udp_length,
	m_udp_dest_port,
	m_udp_src_port,
	m_udp_dest_ip,
	m_udp_src_ip
} = tx_udp_hdr_info;

rx_64_inst
udp_rx_inst (
	.ap_clk(clk),
	.ap_rst_n(~rst),
	// UDP frame input
	.rx_header_V_TDATA(rx_udp_hdr_info),
	.rx_header_V_TVALID(s_udp_hdr_valid),
	.rx_header_V_TREADY(s_udp_hdr_ready),
	.rx_payload_TDATA(s_udp_payload_axis_tdata),
	.rx_payload_TVALID(s_udp_payload_axis_tvalid),
	.rx_payload_TREADY(s_udp_payload_axis_tready),
	.rx_payload_TLAST(s_udp_payload_axis_tlast),
	.rx_payload_TKEEP(s_udp_payload_axis_tkeep),
	.rx_payload_TUSER(s_udp_payload_axis_tuser),
	// UDP frame output
	.rsp_header_V_TDATA(tx_udp_hdr_info),
	.rsp_header_V_TVALID(m_udp_hdr_valid),
	.rsp_header_V_TREADY(m_udp_hdr_ready),
	.rsp_payload_TDATA(m_udp_payload_axis_tdata),
	.rsp_payload_TVALID(m_udp_payload_axis_tvalid),
	.rsp_payload_TREADY(m_udp_payload_axis_tready),
	.rsp_payload_TLAST(m_udp_payload_axis_tlast),
	.rsp_payload_TKEEP(m_udp_payload_axis_tkeep),
	.rsp_payload_TUSER(m_udp_payload_axis_tuser)
);

endmodule
