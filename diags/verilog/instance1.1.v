module main;

reg    a, b;

initial begin
	$dumpfile( "instance1.vcd" );
	$dumpvars( 0, main );
	a = 1'b0;
	b = 1'b0;
	#5;
	a = 1'b1;
	#5;
	b = 1'b1;
	#5;
	a = 1'b0;
	#5;
	$finish;
end

depth1 inst0( a, b );

endmodule

module depth1( a1, b1 );

input    a1;
input    b1;

wire     c1;

assign c1 = a1 & b1;

depth2 inst1( a1, b1 );

endmodule

module depth2( a2, b2 );

input    a2;
input    b2;

wire     c2;

assign c2 = a2 ^ b2;

endmodule
