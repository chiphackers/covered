/*
 Name:        rshift4.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/30/2008
 Purpose:     Verifies right-shift functionality.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [65:0] a;

initial begin
	a = 66'b111xz01xz01xz01xz01xz01xz01xz01xz0_1xz01xz01xz01xz01xz01xz01xz01xz0;
	#5;
	a = a >> 2;
end

initial begin
`ifdef DUMP
        $dumpfile( "rshift4.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
