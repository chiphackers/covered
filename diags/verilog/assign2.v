module main;

reg	    a, b;

wire        w0 = a ^ b;
wire [31:0] w1 = a << b;
wire        w2 = ~a;
wire [1:0]  w3 = w0 & |w1 & w2;

initial begin
`ifdef DUMP
	$dumpfile( "assign2.vcd" );
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
