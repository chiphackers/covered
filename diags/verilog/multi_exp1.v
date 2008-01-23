module main;

reg  b, c, d, e, f, g;

wire a0 = b &  c &  d;
wire a1 = b && c && d;
wire a2 = e |  f |  g;
wire a3 = e || f || g;

initial begin
`ifdef DUMP
	$dumpfile( "multi_exp1.vcd" );
	$dumpvars( 0, main );
`endif
	#10;
	$finish;
end

endmodule
