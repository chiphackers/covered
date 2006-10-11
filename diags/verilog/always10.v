module main;

reg  clock;
reg  a, b, c;

always @(posedge clock)
  if( a )
    b <= c;

initial begin
`ifndef VPI
	$dumpfile( "always10.vcd" );
	$dumpvars( 0, main );
`endif
	a = 1'b0;
	c = 1'b0;
	#20;
	@(negedge clock) a <= 1'b1;
	@(posedge clock) a <= 1'b0;
	#20;
	$finish;
end

initial begin
	clock = 1'b0;
	forever #(2) clock = ~clock;
end

endmodule
