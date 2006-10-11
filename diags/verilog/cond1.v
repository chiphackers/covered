module main;

wire    a;
reg     b;
reg     c, d;

assign a = b ? c : d;

initial begin
`ifndef VPI
        $dumpfile( "cond1.vcd" );
        $dumpvars( 0, main );
`endif
        b = 1'b1;
        c = 1'b0;
        d = 1'b1;
	#5;
        c = 1'b1;
        d = 1'b0;
        #5;
        b = 1'b0;
        #5;
        c = 1'b0;
        d = 1'b1;
	#5;
	b = 1'b1;
	#5;
end

endmodule
