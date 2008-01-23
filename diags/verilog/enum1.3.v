module main;

enum {A, B, C} foo;
enum {D, E} bar;

initial begin
	foo = A;
	bar = D;
	#10;
	foo = C;
        bar = E;
end

initial begin
`ifdef DUMP
        $dumpfile( "enum1.3.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
