/*
 Name:        lshift4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/28/2008
 Purpose:     
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [63:0] a;

initial begin
	a = 64'b1xz01xz01xz01xz01xz01xz01xz01xz0_1xz01xz01xz01xz01xz01xz01xz01xz0;
	#5;
	a = a << 1;
end

initial begin
`ifdef DUMP
        $dumpfile( "lshift4.vcd" );
        $dumpvars( 0, main );
`endif
        #400;
        $finish;
end

endmodule
