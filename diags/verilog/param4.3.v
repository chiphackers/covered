module main;

parameter major = 1;

reg	x, y;

initial begin
	$dumpfile( "param4.3.vcd" );
	$dumpvars( 0, main );
	x = 1'b0;
	y = 1'b0;
	#5;
	x = 1'b1;
	#5;
	x = 1'b0;
	#5;
	$finish;
end

foo #(major) bar1( x );
foo #(major ^ 1'b1) bar2( y );

endmodule

module foo( b );
input	b;
parameter alpha = 1'b0;
wire	a;

assign a = b ^ alpha;

endmodule
