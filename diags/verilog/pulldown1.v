module main;

reg     b;
wire    a = b;

pulldown( a );

initial begin
	$dumpfile( "pulldown1.vcd" );
	$dumpvars( 0, main );
	b = 1'bz;
	#5;
	b = 1'b1;
	#5;
end

endmodule
