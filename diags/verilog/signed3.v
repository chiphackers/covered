module main;

reg a, b, c, d, e, f, g, h;

initial begin
        a = (4'sb1110 == 8'sb11111110);
	b = (8'sb11111110 == 4'sb1110);
	c = (5'b11110 == 4'b1110);
        d = (4'b1110 == 4'sb1110);
	e = -10 < 0;
	f = -10 < -20;
	g = 0 < 4;
	h = -10 < 10;
end

initial begin
`ifdef DUMP
        $dumpfile( "signed3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
