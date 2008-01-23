`define ONE2ZERO 1

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
`ifdef DUMP
        $dumpfile( "elsif1.4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
