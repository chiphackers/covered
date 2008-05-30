/*
 Name:        bitwise1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/23/2008
 Purpose:     
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [63:0] a, b, c, d, e, f;

initial begin
	a = 64'hffffffff_00000000;
	b = 64'hffff0000_ffff0000;
	#5;
	c = a & b;
	d = a | b;
	e = a ^ b;
	f = ~a;
end

initial begin
`ifdef DUMP
        $dumpfile( "bitwise1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
