module main;

reg [0:2][3:0] a;

initial begin
	a         = 12'hfff;
	#1;
	a[0]      = 1'b1;
	#1;
	a[1:2]    = 2'b10;
	#1;
	a[1][0]   = 1'b1;
	#1;
	a[1][3:2] = 2'b11;
end

initial begin
`ifdef DUMP
        $dumpfile( "marray1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
