/*
 Name:        finish1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/26/2008
 Purpose:     Verifies that $finish statements work correctly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#20;
	$finish;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "finish1.vcd" );
        $dumpvars( 0, main );
`endif
end

endmodule
