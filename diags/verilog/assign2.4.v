module main;

reg	    a, b;

trior        w0 = a ^ b;
trior [31:0] w1 = a << b;
trior        w2 = ~a;
trior [1:0]  w3 = w0 & |w1 & w2;

initial begin
`ifdef DUMP
	$dumpfile( "assign2.4.vcd" );
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
