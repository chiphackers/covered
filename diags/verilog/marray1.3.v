module main;

reg [2:0][0:3] a;

initial begin
	a         = 12'hfff;
	a[0]      = 1'b1;
	a[2:1]    = 2'b10;
	a[1][0]   = 2'b1;
        a[1][2:3] = 2'b11;
end

initial begin
`ifndef VPI
        $dumpfile( "marray1.3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
