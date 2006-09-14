`define FOOBAR 1

module main;

`ifdef FOOBAR
`define VALUE0    1'b0
`define VALUE1    1'b1
`else
`define VALUE0    1'b1
`define VALUE1    1'b0
`endif

wire a;
reg  b;

assign a = b ? `VALUE0 : `VALUE1;

initial begin
	$dumpfile( "ifdef2.1.vcd" );
	$dumpvars( 0, main );
	b = 1'b0;
	#5;
	b = 1'b1;
	#5;
	$finish;
end

endmodule
