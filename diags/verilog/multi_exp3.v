module main;

reg [1:0] b, c, d, e;

wire a = (b != 1'b0) & (~c & ~d & ~e);

initial begin
	$dumpfile( "multi_exp3.vcd" );
	$dumpvars( 0, main );
	b = 2'b0;
	c = 2'b0;
	d = 2'b0;
	#10;
	$finish;
end

endmodule
