module main;

reg a, b, c;

initial begin
	#1;
        a = 1'b0;
	a = #5 b | c;
	c = 1'b1;
end

initial begin
	b = 1'b1;
	#2;
	b = 1'b0;
end

initial begin
        $dumpfile( "dly_assign1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
