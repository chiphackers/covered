module main;

reg a;

foo f( a );

initial begin
	$dumpfile( "instance7.vcd" );
	$dumpvars( 0, main );
	a = 1'b0;
	#5;
	a = 1'b1;
	#5;
	$finish;
end

endmodule
