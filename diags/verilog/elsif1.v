`define ZERO2ONE 1

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
        $dumpfile( "elsif1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
