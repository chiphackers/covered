module main;

reg a, b, clock;

always @(b) a = b;

always @(posedge clock) a <= b;

initial begin
`ifdef DUMP
        $dumpfile( "race4.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
        #10;
        $finish;
end

initial begin
	clock = 1'b0;
	forever #1 clock = ~clock;
end

endmodule
