`timescale 1 s / 1 s

module ts_module;

reg a;

initial begin
	a = 1'b0;
	#1;
	a = 1'b1;
	#1;
	a = 1'b0;
end

endmodule
