module main;

reg	    a, b;

tri0        w0 = a ^ b;
tri0 [31:0] w1 = a << b;
tri0        w2 = ~a;
tri0 [1:0]  w3 = w0 & |w1 & w2;

initial begin
	$dumpfile( "assign2.2.vcd" );
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
