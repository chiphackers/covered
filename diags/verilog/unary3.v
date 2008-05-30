/*
 Name:        unary3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/24/2008
 Purpose:     Verify that unary XOR/NXOR with X values are calculated properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [79:0] a;
reg        b, c;

initial begin
	b = 1'b0;
        c = 1'b0;
        #5;
        a = 80'hffff_0000ffff_0000ffff;
        b = ^a;
	c = ~^a;
	#5;
	a[20] = 1'bx;
	b = ^a;
	c = ~^a;
	#5;
	$finish;
end

initial begin
`ifdef DUMP
        $dumpfile( "unary3.vcd" );
        $dumpvars( 0, main );
`endif
end

endmodule
