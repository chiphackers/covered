module main;

reg a, b, c;

initial begin
	#1;
	a = 1'b0;
	a = @(posedge b) c;
end

initial begin
	b = 1'b0;
	c = 1'b1;
	#10;
	b = 1'b1;
	c = 1'b0;
end

initial begin
`ifndef VPI
        $dumpfile( "dly_assign1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #50;
        $finish;
end

endmodule
