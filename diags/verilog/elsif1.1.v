`define ONE2ZERO 1

module main;

reg a;

initial begin
`ifdef ZERO2ONE
	a = 1'b0;
	a = 1'b1;
`elsif ONE2ZERO
	a = 1'b1;
	a = 1'b0;
`else
	a = 1'b0;
`endif
end

initial begin
`ifdef DUMP
        $dumpfile( "elsif1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
