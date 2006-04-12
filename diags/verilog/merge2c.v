module main3;

wire    a;
reg     b, c;

dut_and dut( a, b, c );

initial begin
	$dumpfile( "merge2c.vcd" );
	$dumpvars( 0, main3 );
	b = 1'b0;
	c = 1'b1;
	#5;
	$finish;
end

endmodule
