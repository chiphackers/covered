module main;

reg	c, d;

always @(negedge c) d <= ~d;

initial begin
	$dumpfile( "always3.vcd" );
	$dumpvars( 0, main );
	c = 1'b0;
	d = 1'b0;
	forever #(4) c = ~c;
end

initial begin
	#100;
	$finish;
end

endmodule
