module main;

event b;
reg a, c, d;

initial begin
	a = 1'b0;
        d = 1'b0;
	#1;
	a = repeat(2) @(b) c;
	d = 1'b1;
end

initial begin
	c = 1'b1;
	->b;
	#10;
	c = 1'b0;
end

initial begin
`ifndef VPI
        $dumpfile( "dly_assign2.vcd" );
        $dumpvars( 0, main );
`endif
        #50;
        $finish;
end

endmodule
