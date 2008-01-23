module main;

`include "inc_body.v"

initial begin
`ifdef DUMP
	$dumpfile( "include2.vcd" );
	$dumpvars( 0, main );
`endif
end

endmodule
