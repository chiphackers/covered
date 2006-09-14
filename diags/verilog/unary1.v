module main;

reg [2:0] a;

wire b = ~a;
wire c = !a;
wire d = &a;
wire e = ~&a;
wire f = |a;
wire g = ~|a;
wire h = ^a;
wire i = ~^a;
wire j = !(|a);

initial begin
	$dumpfile( "unary1.vcd" );
	$dumpvars( 0, main );
	a = 3'h0;
	#20;
	$finish;
end

endmodule
