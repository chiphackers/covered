module main2;

wire    a;
reg     b, c;

dut_and dut( a, b, c );

initial begin
	$dumpfile( "merge1b.vcd" );
	$dumpvars( 0, main2 );
	b = 1'b1;
	c = 1'b0;
	#5;
	$finish;
end

endmodule
