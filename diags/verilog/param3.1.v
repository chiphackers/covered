module main;

reg	x, y;

initial begin
`ifdef DUMP
	$dumpfile( "param3.1.vcd" );
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

foo #(0) bar1( x );
foo #(1) bar2( y );

endmodule

module foo( b );
parameter alpha = 1'b0;
input	b;
wire	a;

assign a = b ^ alpha;

endmodule
