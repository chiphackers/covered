module main;

reg   b;
reg   a;

always @(b) a = b & (0.1 == 0.2);

initial begin
	$dumpfile( "real1.1.vcd" );
	$dumpvars( 0, main );
	b = 1'b0;
	#5;
	b = 1'b1;
	#5;
	$finish;
end

endmodule
