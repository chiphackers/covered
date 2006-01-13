module main;

integer a, b, c;

initial begin
	b = 2;
	c = 3;
	#1;
	a = b ** c;
	a = b ** c;
end

initial begin
        $dumpfile( "exponent1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
