module main;

reg	    a, b;

triand        w0 = a ^ b;
triand [31:0] w1 = a << b;
triand        w2 = ~a;
triand [1:0]  w3 = w0 & |w1 & w2;

initial begin
`ifndef VPI
	$dumpfile( "assign2.5.vcd" );
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
