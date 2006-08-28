module main;

reg a, b, c, d, e;

initial begin
	#1;
	a = 1'b0;
	a = @(posedge c or negedge d or e) b;
end

initial begin
	b = 1'b1;
	c = 1'b0;
	d = 1'b0;
        e = 1'b0;
	#10;
	e = 1'b1;
	c = 1'b0;
end

initial begin
        $dumpfile( "dly_assign1.4.vcd" );
        $dumpvars( 0, main );
        #50;
        $finish;
end

endmodule
