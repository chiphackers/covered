module main;

reg  b, c, d, e, f, g;
wire a;

foobar bar( c );

assign a = ~b & (c ^ d) & (e | (f == 1'b1) | g);

initial begin
	$dumpfile( "multi_exp2.vcd" );
	$dumpvars( 0, main );
	b = 1'b0;
	c = 1'b0;
	d = 1'b0;
	e = 1'b0;
	f = 1'b0;
	g = 1'b0;
	#10;
	g = 1'b1;
	d = 1'b1;
	#10;
	$finish;
end

endmodule

module foobar( c );

input c;
wire  c;
reg   a;

always @(posedge c) a <= 1'b1;

endmodule
