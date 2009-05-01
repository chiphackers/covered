/*
 Name:        race7.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/25/2009
 Purpose:     Verify that the -rI=<module name> option works properly when more than one is specified.
*/

module main;

reg x;

foo a( 1'b0 );
bar b( 1'b0 );

initial begin
`ifdef DUMP
        $dumpfile( "race7.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule

module foo(
  input a
);

reg b;

initial begin
	b = a;
end

initial begin
	#5;
	b = ~a;
end

endmodule

module bar(
  input a
);

reg b;

initial begin
	b = ~a;
end

initial begin
	#5;
	b = a;
end

endmodule
