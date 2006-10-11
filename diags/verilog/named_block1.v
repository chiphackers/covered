module main;

reg a, b;

initial begin : foobar
	a = 1'b0;
end

initial begin : barfoo
	b <= 1'b1;
end

initial begin
	#10;
	begin : fooey
	 reg c, d;
	 c <= a & b;
	 d <= a | b;
	end
	begin : barry
         reg e, f;
	 e <= a ^ b;
	 f <= a ~^ b;
	end
end

initial begin
`ifndef VPI
        $dumpfile( "named_block1.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

endmodule
