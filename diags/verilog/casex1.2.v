module main;

reg	a, b;

always @( a )
  casex( a )
    1'b0 :  b = 1'b1;
    1'b1 :  b = 1'b0;
    1'bx :  b = 1'b1;
    1'bz :  b = 1'b0;
  endcase

initial begin
	$dumpfile( "casex1.2.vcd" );
	$dumpvars( 0, main );
	#5;
	a = 1'b0;
	#5;
	a = 1'bx;
	#5;
	a = 1'bz;
	#5;
	$finish;
end

endmodule
