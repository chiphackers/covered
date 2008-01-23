module main;

reg [31:0] a, b, c;

initial begin
	b = 2;
	c = 3;
	#1;
	a = b ** c;
	a = a ** c;
end

initial begin
`ifdef DUMP
        $dumpfile( "exponent1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
