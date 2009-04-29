/*
 Name:        merge8.11a.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/28/2009
 Purpose:     
*/

module main;

wire a;
reg  b, c;

initial begin
	b = 1'b0;
	c = 1'b0;
	#5;
	c = 1'b1;
end

dut_nand_proc dut( a, b, c );

initial begin
`ifdef DUMP
        $dumpfile( "merge8.11a.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule

module dut_nand_proc(
  output reg  a,
  input  wire b,
  input  wire c
);

wire x;

dut_and_proc and_proc( x, b, c );

always @* begin
  a = ~x;
end

endmodule
