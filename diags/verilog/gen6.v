module main;

reg [31:0] a, b, c, d;

initial begin
	a = 2;
	b = 3;
	c = a ** b;
	d = c;
end

endmodule
