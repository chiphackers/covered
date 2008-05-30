/*
 Name:        lshift2.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/28/2008
 Purpose:     Verifies left-shifting for values of MOD 32.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [125:0] a;

initial begin
	a = 126'b1xz01;
	#5;
	a = a << 64;
end

initial begin
`ifdef DUMP
        $dumpfile( "lshift2.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
