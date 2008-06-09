/*
 Name:        err7.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/08/2008
 Purpose:     Verify that empty CDD file is flagged as an error to the
              user and that Covered handles the memory management properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
	#5;
	$finish;
end

initial begin
`ifdef DUMP
	$dumpfile( "err7.1.vcd" );
	$dumpvars( 0, main );
`endif
end

endmodule
