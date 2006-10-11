module main;

reg  [1:0]   a;
reg          b;
reg          c;
reg  [3:0]   d;

always @(d) {a, b, c} = d;

initial begin
`ifndef VPI
	$dumpfile( "concat4.2.vcd" );
	$dumpvars( 0, main );
`endif
	d = 4'h0;
	#5;
	d = 4'h1;
	#5;
	d = 4'h2;
	#5;
	d = 4'h4;
	#5;
	d = 4'h8;
	#5;
	$finish;
end

endmodule
