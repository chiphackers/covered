	////////////////////////////////////////////////////////////////////////////// 
	//
	//  ODIN Control Logic
	//
	//  Original Author: Brian Holscher
	//  Current Owner:   Brian Holscher
	//
	////////////////////////////////////////////////////////////////////////////// 
	//
	// Copyright (C) 2002 Accelerant Networks, Inc
	// Licensed material -- Program property of Accelerant Networks, Inc.
	// All Rights Reserved
	//
	// This program is the property of Accelerant Networks and is furnished
	// pursuant to a written license agreement. It may not be used, reproduced,
	// or disclosed to others except in accordance with the terms and conditions
	// of that agreement.
	//
	////////////////////////////////////////////////////////////////////////////// 
	//
	// CVS Information:
	//
	//   $Author$
	//   $Source$
	//   $Date$
	//   $Revision$
	//
	////////////////////////////////////////////////////////////////////////////// 
	////////////////////////////////////////////////////////////////////////////// 
	//
	//  Constants
	//
	////////////////////////////////////////////////////////////////////////////// 
	`include "ani_macros.v"
	`include "thor_macros.v"
	`include "odin_macros.v"
	////////////////////////////////////////////////////////////////////////////// 
	//
	//  Main
	//
	////////////////////////////////////////////////////////////////////////////// 
	module odin_ctl (odin_req, chan_ack, odin_adr, odin_wr_data, odin_rw, 
	   mdio_cr_req, mdio_cr_adr, mdio_cr_wr_data, mdio_cr_rw,
	   jtag_cr_req, jtag_cr_adr, jtag_cr_wr_data, jtag_cr_rw,
	   micro_req, micro_ack, micro_adr, micro_wr_data, micro_rw,
	   user_reg_write, drive_ads, 
	   p4_rt_req, p4_rt_ack,
	   cml_rt_req, cml_rt_ack,
	   odin_rd_data, jtag_cr_rd_data, mdio_rd_data,
	   jtag_cr_ack, crm_state_adr, odin_ck, odin_reset);
	 
	   output                    odin_req;
	   input        chan_ack;
	   output [`ODIN_ADR_RANGE]  odin_adr;
	   output [`ODIN_DATA_RANGE] odin_wr_data;
	   output        odin_rw;
	   
	   input        mdio_cr_req;
	   input [`ODIN_ADR_RANGE]   mdio_cr_adr;
	   input [`ODIN_DATA_RANGE]  mdio_cr_wr_data;
	   input        mdio_cr_rw;
	   input        jtag_cr_req;
	   input [`ODIN_ADR_RANGE]   jtag_cr_adr;
	   input [`ODIN_DATA_RANGE]  jtag_cr_wr_data;
	   input        jtag_cr_rw;
	   input        micro_req;
	   output        micro_ack;
	   input [`ODIN_ADR_RANGE]   micro_adr;
	   input [`ODIN_DATA_RANGE]  micro_wr_data;
	   input        micro_rw;
	   output        user_reg_write;
	   output [3:0]       drive_ads;
	   input        p4_rt_req;
	   output        p4_rt_ack;
	   input        cml_rt_req;
	   output        cml_rt_ack;
	   input [`ODIN_DATA_RANGE]  odin_rd_data;
	   output [`ODIN_DATA_RANGE] jtag_cr_rd_data;
	   output [`ODIN_DATA_RANGE] mdio_rd_data;
	   output        jtag_cr_ack;
	   input        crm_state_adr;
	   input        odin_ck;
	   input        odin_reset;
	   wire [4:0]       requests;
	   reg [4:0]       active, next_req;
	   
	   reg [`ODIN_ADR_RANGE]  odin_adr;
	   reg [`ODIN_DATA_RANGE] odin_wr_data;
	   reg      odin_rw;
	   reg [`ODIN_DATA_RANGE] mdio_rd_data;
	   wire     next_active;
	   
	   /////////////////////////////////////////////////////////////////////////// 
	   //
	   // Arbitration Logic
	   //
	   /////////////////////////////////////////////////////////////////////////// 
	   `define REQ_MDIO  4
	   `define REQ_JTAG  3
	   `define REQ_P4    2
	   `define REQ_CML   1
	   `define REQ_MICRO 0
	   // generic_sync mdio_req_sync(mdio_cr_req_s02, odin_ck, mdio_cr_req);
	   // generic_sync jtag_req_sync(jtag_cr_req_s02, odin_ck, jtag_cr_req);
	   assign requests = {mdio_cr_req_s02, jtag_cr_req_s02, 
	        p4_rt_req, cml_rt_req, micro_req};
	   
	   
	   always @(requests) begin
	      casez (requests)
	 5'b1???? : next_req = 5'b10000;
	 5'b01??? : next_req = 5'b01000;
	 5'b001?? : next_req = 5'b00100;
	 5'b0001? : next_req = 5'b00010;
	 5'b00001 : next_req = 5'b00001;
	 default   : next_req = 5'b00000;
	      endcase // casez(req)
	   end
	   assign next_active = | (~requests & active);
	   
	   always @(posedge odin_reset or posedge odin_ck) begin
	      if (odin_reset)
	 active <= 5'h00;
	      else if ( next_active | (active == 5'h00))
	 active <= next_req;
	   end
	   
	   
	   assign odin_req = (active[`REQ_MDIO] & mdio_cr_req) | 
	       (active[`REQ_JTAG] & jtag_cr_req) |
	       (active[`REQ_MICRO] & micro_req);

initial begin
	$dumpfile( "define5.vcd" );
	$dumpvars( 0, odin_ctl );
	#100;
	$finish;
end
	

endmodule
