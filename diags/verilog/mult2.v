/*
 Name:        mult2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/24/2008
 Purpose:     Verify that a value of X is computed for two values being multiplied
              where at least one value contains an X-value.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [31:0] a, b, c;

initial begin
	a = 0;
	#5;
        b = 4;
        c = 5;
	a = b * c;
	#5;
        b[10] = 1'bz;
	a = b * c;
	#5;
	$finish;
end

initial begin
`ifdef DUMP
        $dumpfile( "mult2.vcd" );
        $dumpvars( 0, main );
`endif
end

endmodule
