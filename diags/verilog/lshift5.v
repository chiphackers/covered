/*
 Name:        lshift5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/24/2008
 Purpose:     Verify that the correct thing happens when the shift value is X.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [39:0] a;
reg [31:0] b;

initial begin
	a = 40'h1;
	#5;
	b = 32'd1;
	a = a << b;
	#5;
	b[20] = 1'bx;
	a = a << b;
	#5;
	$finish;
end

initial begin
`ifdef DUMP
        $dumpfile( "lshift5.vcd" );
        $dumpvars( 0, main );
`endif
end

endmodule
