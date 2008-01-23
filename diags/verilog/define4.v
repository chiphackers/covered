/*
 * This diagnostic verifies the appropriate behavior when the EOF is detected
 * after a define statement (a newline is not detected after the define).  In
 * this case, Covered should check in its yywrap function if we were in the middle
 * of defining and, if so, create the define.
 */

`include "define4.h"

module main;

reg     b;
wire    a = b & `FOOBAR;

initial begin
`ifdef DUMP
	$dumpfile( "define4.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b0;
	#5;
	b = 1'b1;
	#5;
	$finish;
end

endmodule
