module main;

parameter foo = 5;

initial begin : bar
	reg [foo-1:0] a;
	a = 5'h0;
end

initial begin
`ifndef VPI
        $dumpfile( "param10.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
