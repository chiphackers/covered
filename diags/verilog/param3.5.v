module main;

reg	x, y;

initial begin
`ifndef VPI
	$dumpfile( "param3.vcd" );
	$dumpvars( 0, main );
`endif
	x = 1'b0;
	y = 1'b0;
	#5;
	x = 1'b1;
	#5;
	x = 1'b0;
	#5;
	$finish;
end

foo bar1( x );
foo bar2( y );

endmodule

module foo( b );
input	b;
parameter alpha = 1'b0;
wire	a;

assign a = b ^ alpha;

endmodule
