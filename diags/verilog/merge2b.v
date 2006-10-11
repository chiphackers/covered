module main2;

wire    a;
reg     b, c;

dut_and dut( a, b, c );

initial begin
`ifndef VPI
	$dumpfile( "merge2b.vcd" );
	$dumpvars( 0, main2 );
`endif
	b = 1'b1;
	c = 1'b0;
	#5;
	$finish;
end

endmodule
