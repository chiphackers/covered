module main;

reg [0:2][0:3] a;

initial begin
	a         = 12'hfff;
	a[0]      = 1'b1;
	a[1:2]    = 2'b10;
	a[1][0]   = 1'b1;
	a[1][2:3] = 2'b11;
end

initial begin
`ifndef VPI
        $dumpfile( "marray1.2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
