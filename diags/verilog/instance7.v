module main;

reg a;

foo f( a );

initial begin
`ifndef VPI
	$dumpfile( "instance7.vcd" );
	$dumpvars( 0, main );
`endif
	a = 1'b0;
	#5;
	a = 1'b1;
	#5;
	$finish;
end

endmodule
