module main;

`define WIDTH  4
`define VALUE  10

wire   [`WIDTH-1:0] a, b;
reg    [`WIDTH-1:0] c;

assign b = `WIDTH'd10 + c;
assign a = `WIDTH'd`VALUE + c;

initial begin
	$dumpfile( "define3.vcd" );
	$dumpvars( 0, main );
	c = `WIDTH'd2;
	#5;
	c = `WIDTH'd3;
	#5;
	$finish;
end

endmodule
