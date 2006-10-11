module main;

reg   b;
reg   a;

always @(b)
  a = b & (0.1 == 0.2);

initial begin
`ifndef VPI
	$dumpfile( "real1.1.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b0;
	#5;
	b = 1'b1;
	#5;
	$finish;
end

endmodule
