module main;

reg [3:0] a, b, c;

initial begin
	a = 4'h0;
	a = 4'h1 <<< 3;
end

initial begin
	b = 4'h0;
	b = 4'h4 >>> 2;
end

initial begin
	c = 4'h0;
	c = 4'h8 >>> 2;
end

initial begin
`ifdef DUMP
        $dumpfile( "ashift1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
