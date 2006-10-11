module main;

wire [1:0]   a;
reg          b;
reg          c;

assign a = { (b | c), (b & c) };

initial begin
`ifndef VPI
	$dumpfile( "concat2.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b1;
	c = 1'b0;
	#5;
	b = 1'b0;
	#5;
	c = 1'b1;
	#5;
	$finish;
end

endmodule
