module main;

reg	a, b;

always @(a)
  case( a )
    1'b0 :  b = 1'b1;
    1'b1 :  b = 1'b0;
    1'bx :  b = 1'b1;
    1'bz :  b = 1'b0;
  endcase

initial begin
	$dumpfile( "case3.1.vcd" );
	$dumpvars( 0, main );
	#5;
	a = 1'b1;
	#5;
        a = 1'bx;
        #5;
        a = 1'bz;
	#5;
	$finish;
end

endmodule
