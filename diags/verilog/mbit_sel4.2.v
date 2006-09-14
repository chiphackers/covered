module main;

reg  [4:1] a;

initial begin
	$dumpfile( "mbit_sel4.2.vcd" );
	$dumpvars( 0, main );
	a[1] = 1'b1;
	a[2] = 1'b0;
	a[3] = 1'b1;
	a[4] = 1'b1;
	#5;
	a[2] = 1'b1;
	a[4] = 1'b0;
	#5;
	$finish;
end

endmodule
