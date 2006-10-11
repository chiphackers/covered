module main;

parameter BAR = 1;

enum { A=BAR, B, C } foo;

initial begin
	foo = A;
	#5;
	foo = B;
end

initial begin
`ifndef VPI
        $dumpfile( "enum1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
