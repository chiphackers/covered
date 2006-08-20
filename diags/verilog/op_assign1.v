module main;

reg [3:0] a, b, c, d, e, f, g, h, i, j, k, l;

initial begin
	a = 0;
	a += 2;
end

initial begin
	b = 0;
	b -= 2;
end

initial begin
	c = 1;
	c *= 2;
end

initial begin
	d = 2;
	d /= 2;
end

initial begin
	e = 2;
	e %= 2;
end

initial begin
	f = 1;
	f <<= 2;
end

initial begin
	g = 8;
	g >>= 2;
end

initial begin
	h = 1;
	h <<<= 2;
end

initial begin
	i = 8;
	i >>>= 2;
end

initial begin
	j = 15;
	j &= 2;
end

initial begin
	k = 0;
	k |= 2;
end

initial begin
	l = 2;
	l ^= 3;
end

initial begin
        $dumpfile( "op_assign1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
