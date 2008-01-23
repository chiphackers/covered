module main;

reg  b, c, d;

wire a = b & c & d;
wire e = 1'b0 & b;

initial begin
`ifdef DUMP
	$dumpfile( "multi_exp2.1.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b0;
	c = 1'b0;
	d = 1'b0;
	#10;
	b = 1'b1;
	c = 1'b1;
	d = 1'b1;
	#10;
	$finish;
end

endmodule
