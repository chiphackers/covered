module main;

reg	    a, b;

supply1        w0 = a ^ b;
supply1 [31:0] w1 = a << b;
supply1        w2 = ~a;
supply1 [1:0]  w3 = w0 & |w1 & w2;

initial begin
`ifdef DUMP
	$dumpfile( "assign2.9.vcd" );
        $dumpvars( 0, main );
`endif
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
