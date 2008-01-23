module main;

reg  b, c, d, e, f, g;

wire a0 = b &  c &  d;
wire a1 = b && c && d;
wire a2 = e |  f |  g;
wire a3 = e || f || g;

initial begin
`ifdef DUMP
	$dumpfile( "multi_exp1.8.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b1;
	c = 1'b1;
	d = 1'b1;
	e = 1'b0;
	f = 1'b0;
	g = 1'b0;
	#10;
	$finish;
end

endmodule
