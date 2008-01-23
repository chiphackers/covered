module main;

enum shortint {A, B, C} foo;

initial begin
	foo = B;
	#10;
	foo = A;
end

initial begin
`ifdef DUMP
        $dumpfile( "enum1.2.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
