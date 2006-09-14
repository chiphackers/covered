module main;

`include "inc_body.v"

initial begin
	$dumpfile( "include2.vcd" );
	$dumpvars( 0, main );
end

endmodule
