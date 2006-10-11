module main;

reg [1:0] a;

initial begin
	a = 1'b0;
	a--;
	@(a);
	a--;
end

initial begin
`ifndef VPI
        $dumpfile( "dec1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
