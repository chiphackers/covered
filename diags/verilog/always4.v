module main;

reg	c, d;

always @(posedge c or negedge c) d <= ~d;

initial begin
`ifndef VPI
	$dumpfile( "always4.vcd" );
	$dumpvars( 0, main );
`endif
	c = 1'b0;
	d = 1'b0;
	forever #(4) c = ~c;
end

initial begin
	#100;
	$finish;
end

endmodule
