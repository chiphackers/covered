module main;

reg [31:0] b, c;
reg [3:0]  a;

initial begin
	b = 32'h01234567;
	c = 4;
	a = b[c+:4];
end

endmodule
