module main;

reg     b;
wire    a = b;

pulldown( a );

initial begin
`ifndef VPI
	$dumpfile( "pulldown1.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'bz;
	#5;
	b = 1'b1;
	#5;
end

endmodule
