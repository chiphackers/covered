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
	#10;
	->b;
	#10;
	c = 1'b0;
	->b;
end

initial begin
`ifdef DUMP
        $dumpfile( "dly_assign2.1.vcd" );
        $dumpvars( 0, main );
`endif
        #50;
        $finish;
end

endmodule
