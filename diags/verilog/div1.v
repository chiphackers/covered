/*
 Name:        div1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/24/2008
 Purpose:     Verify that a divide by zero causes the target value to be assigned an X value.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

integer a, b;

initial begin
	a = 0;
        b = 0;
	#5;
	b = 32 / a;
	#5;
	$finish;
end

initial begin
`ifdef DUMP
	$dumpfile( "div1.vcd" );
	$dumpvars( 0, main );
`endif
end

endmodule
