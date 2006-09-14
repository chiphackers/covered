`include "diag.h"

module main;

wire    a;
reg     b;

assign a = b ? `VALUE0 : `VALUE1;

initial begin
	$dumpfile( "include1.vcd" );
	$dumpvars( 0, main );
	#5;
	b = 1'b0;
	#5;
	b = 1'b1;
	#5;
	$finish;
end

endmodule
