module main;

reg a, b;

always @(b) a = b;

initial begin
`ifndef VPI
        $dumpfile( "race3.1.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1'b0;
	b = 1'b1;
        #10;
        $finish;
end

endmodule
