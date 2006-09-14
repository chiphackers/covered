module main;

reg	    a, b;

tri        w0 = a ^ b;
tri [31:0] w1 = a << b;
tri        w2 = ~a;
tri [1:0]  w3 = w0 & |w1 & w2;

initial begin
	$dumpfile( "assign2.1.vcd" );
        $dumpvars( 0, main );
        a = 1'b0;
        b = 1'b0;
	#5;
	a = 1'b1;
	#5;
	b = 1'b1;
	#5;
	$finish;
end

endmodule
