/*
 Name:        ashift4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/24/2008
 Purpose:     Verify that the correct thing happens when the arithmetic right shift value is X.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg signed [39:0] a;
reg [31:0] b;

initial begin
	a = 40'h8000000000;
	#5;
	b = 32'd1;
	a = a >>> b;
	#5;
	b[20] = 1'bx;
	a = a >>> b;
	#5;
	$finish;
end

initial begin
`ifdef DUMP
        $dumpfile( "ashift4.vcd" );
        $dumpvars( 0, main );
`endif
end

endmodule
