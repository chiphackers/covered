`include "multi.h"

module main;

`include "inc_body.v"

initial begin
	$dumpfile( "include3.vcd" );
	$dumpvars( 0, main );
end

endmodule
