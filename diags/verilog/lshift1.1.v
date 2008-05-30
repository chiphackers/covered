/*
 Name:        lshift1.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/28/2008
 Purpose:     Verifies that left-shift works correctly for vectors less than 32-bits.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [15:0] a;

initial begin
	a = 16'b1xz0;
	#5;
	a = a << 13;
end

initial begin
`ifdef DUMP
        $dumpfile( "lshift1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
