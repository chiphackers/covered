module main;

parameter BAR = 1;

enum [1:0] { A=BAR, B, C } foo;

initial begin
	foo = A;
	#5;
	foo = B;
end

initial begin
`ifdef DUMP
        $dumpfile( "enum1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
