module main;

integer  i;
time     t;

initial begin
	$dumpfile( "implicit2.vcd" );
	$dumpvars( 0, main );
	i  = 1;
	t  = 2;
	#5;
	i  = 10;
	t  = 20;
	#5;
	$finish;
end

endmodule
