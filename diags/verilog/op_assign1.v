module main;

reg [3:0] a, b, c, d, e, f, g, h, i, j, k, l;

initial begin
	a = 0;
	#5;
	a += 2;
end

initial begin
	b = 0;
	#5;
	b -= 2;
end

initial begin
	c = 1;
	#5;
	c *= 2;
end

initial begin
	d = 2;
	#5;
	d /= 2;
end

initial begin
	e = 2;
	#5;
	e %= 2;
end

initial begin
	f = 1;
	#5;
	f <<= 2;
end

initial begin
	g = 8;
	#5;
	g >>= 2;
end

initial begin
	h = 1;
	#5;
	h <<<= 2;
end

initial begin
	i = 8;
	#5;
	i >>>= 2;
end

initial begin
	j = 15;
	#5;
	j &= 2;
end

initial begin
	k = 0;
	#5;
	k |= 2;
end

initial begin
	l = 2;
	#5;
	l ^= 3;
end

initial begin
`ifdef DUMP
        $dumpfile( "op_assign1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
