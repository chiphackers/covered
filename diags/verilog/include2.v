module main;

`include "inc_body.v"

initial begin
`ifndef VPI
	$dumpfile( "include2.vcd" );
	$dumpvars( 0, main );
`endif
end

endmodule
