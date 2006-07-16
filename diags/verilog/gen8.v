module main;

reg [31:0] a, b;

initial begin
	b = 36;
	a = b <<< 3;
end

endmodule
