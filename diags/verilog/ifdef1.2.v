`define FOOBAR     1

module main;

`define VALUE0    1'b0

`ifdef FOOBAR
`define VALUE0    1'b1
`endif

wire a;
reg  b;

assign a = b & `VALUE0;

initial begin
`ifdef DUMP
	$dumpfile( "ifdef1.2.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b0;
	#5;
	b = 1'b1;
	#5;
	$finish;
end

endmodule
