module main;

reg  a;

wire b = ~a;
wire c = !a;
wire d = &a;
wire e = ~&a;
wire f = |a;
wire g = ~|a;
wire h = ^a;
wire i = ~^a;

initial begin
	$dumpfile( "unary1.vcd" );
	$dumpvars( 0, main );
	a = 1'b0;
	#20;
	$finish;
end

endmodule
