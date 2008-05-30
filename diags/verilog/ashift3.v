/*
 Name:        ashift3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/24/2008
 Purpose:     Verifies that an arithmetic right shift of a large value is sign-extended properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg signed [95:0] a;

initial begin
	a = 96'h80000000_00000000_00000000;
	#5;
	a = a >>> 35;
end

initial begin
`ifdef DUMP
        $dumpfile( "ashift3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
