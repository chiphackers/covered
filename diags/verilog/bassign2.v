module main;

reg [31:0] a, b;

initial begin
	a    = 1;
        b    = 0;
	b[a] = 1;
end

initial begin
`ifdef DUMP
        $dumpfile( "bassign2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
