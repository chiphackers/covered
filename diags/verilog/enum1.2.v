module main;

enum shortint {A, B, C} foo;

initial begin
	foo = B;
	#10;
	foo = A;
end

initial begin
        $dumpfile( "enum1.2.vcd" );
        $dumpvars( 0, main );
        #20;
        $finish;
end

endmodule
