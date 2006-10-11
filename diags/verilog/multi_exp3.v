module main;

reg  b, c, d, e;

wire a = (b != 1'b0) & (~c & ~d & ~e);

initial begin
`ifndef VPI
	$dumpfile( "multi_exp3.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b0;
	c = 1'b0;
	d = 1'b0;
	#10;
	$finish;
end

endmodule
