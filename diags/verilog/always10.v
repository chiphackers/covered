module main;

reg  clock;
reg  a, b, c;

always @(posedge clock)
  if( a )
    b <= c;

initial begin
	$dumpfile( "always10.vcd" );
	$dumpvars( 0, main );
	a = 1'b0;
	c = 1'b0;
	#20;
	@(posedge clock) a <= 1'b1;
	@(posedge clock) a <= 1'b0;
	#20;
	$finish;
end

initial begin
	clock = 1'b0;
	forever #(2) clock = ~clock;
end

endmodule
