/*
 Name:        negate1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/24/2008
 Purpose:     Verify that a negation operation on a value with unknown bits evaluates to X.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [39:0] a, b;

initial begin
	a = 40'h0;
	b = 40'h1;
	#5;
        a = -b;
	#5;
        b[1] = 1'b1;
	b[3] = 1'bx;
	a = -b;
	#5;
	$finish;
end

initial begin
`ifdef DUMP
        $dumpfile( "negate1.vcd" );
        $dumpvars( 0, main );
`endif
end

endmodule
