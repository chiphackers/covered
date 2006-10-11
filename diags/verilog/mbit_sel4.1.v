module main;

reg  [3:0] a;

initial begin
`ifndef VPI
	$dumpfile( "mbit_sel4.vcd" );
	$dumpvars( 0, main );
`endif
	a[0] = 1'b1;
	a[1] = 1'b0;
	a[2] = 1'b1;
	a[3] = 1'b1;
	#5;
	a[1] = 1'b1;
	a[3] = 1'b0;
	#5;
	$finish;
end

endmodule
