module main;

real  a;
reg   b;

always @(b) a = b ? 0.1 : 0.2;

initial begin
`ifndef VPI
	$dumpfile( "real1.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b0;
	#5;
	$finish;
end

endmodule
