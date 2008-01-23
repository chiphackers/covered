module main;

`include "diag.h"

wire    a;
reg     b;

assign a = b ? `VALUE0 : `VALUE1;

initial begin
`ifdef DUMP
	$dumpfile( "include1.1.vcd" );
	$dumpvars( 0, main );
`endif
	#5;
	b = 1'b0;
	#5;
	b = 1'b1;
	#5;
	$finish;
end

endmodule
