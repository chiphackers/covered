module main;

reg a, b;

initial begin
`ifdef ZERO2ONE
	a = 1'b0;
	a = 1'b1;
`elsif ONE2ZERO
	a = 1'b1;
	a = 1'b0;
`endif
	b = 1'b0;
	b = 1'b1;
end

initial begin
        $dumpfile( "elsif1.3.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
