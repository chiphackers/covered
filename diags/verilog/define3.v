module main;

`define WIDTH  4
`define VALUE  10

wire   [`WIDTH-1:0] a, b, c, d;
reg    [`WIDTH-1:0] e;

assign d = 'd10 + e;
assign c = `WIDTH'd10 + e;
assign b = `WIDTH'd`VALUE + e;
assign a = 'd`VALUE + e;

initial begin
`ifdef DUMP
	$dumpfile( "define3.vcd" );
	$dumpvars( 0, main );
`endif
	e = `WIDTH'd2;
	#5;
	e = `WIDTH'd3;
	#5;
	$finish;
end

endmodule
