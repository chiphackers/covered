module main;

reg     b;
wire    a = b;

pullup( a );

initial begin
	$dumpfile( "pullup1.vcd" );
	$dumpvars( 0, main );
	b = 1'bz;
	#5;
	b = 1'b0;
	#5;
end

endmodule
