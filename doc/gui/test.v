module main;

reg a, b, c, d, e, f, g, h, i, j;
reg clock;
reg [3:0] err_vec;

initial begin
	a = 1'b0;
	@(posedge a) a = 1'b1;
end

initial begin
	#1;
	b = ~c;
end

initial begin
	#1;
	d = e & f;
end

initial begin
	#1;
	g = h | i | j;
end

initial begin
	$dumpfile( "test.vcd" );
	$dumpvars( 0, main );
	c = 1'b0;
	e = 1'b1;
	f = 1'b0;
	h = 1'b0;
	i = 1'b1;
	j = 1'b1;
	err_vec = 4'b0000;
	#5;
	err_vec = 4'b1100;
	#10;
end

endmodule
