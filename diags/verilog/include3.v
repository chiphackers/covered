`include "multi.h"

module main;

`include "inc_body.v"

initial begin
`ifdef DUMP
	$dumpfile( "include3.vcd" );
	$dumpvars( 0, main );
`endif
end

endmodule
