module main;

reg a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p;
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
	g = h | i;
end

initial begin
	#1;
	j = k ^ l;
end

initial begin
	#1;
	m = n | o | p;
end

initial begin
	$dumpfile( "test.vcd" );
	$dumpvars( 0, main );
	c = 1'b0;
	e = 1'b1;
	f = 1'b0;
        h = 1'b1;
        i = 1'b0;
        k = 1'b0;
        l = 1'b1;
	n = 1'b0;
	o = 1'b1;
	p = 1'b1;
	err_vec = 4'b0000;
	#5;
	err_vec = 4'b1100;
	#10;
end

endmodule
