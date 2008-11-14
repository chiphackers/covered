/*
 Name:        merge8.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/31/2008
 Purpose:     Verifies that different modules from the same design can be merged.
*/

module main;

reg a, b;

foo m1( a, b );
bar m2( b );

initial begin
`ifdef DUMP
        $dumpfile( "merge8.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1'b0;
	b = 1'b1;
        #10;
        $finish;
end

endmodule

//-------------------------------------------------

module foo(
  input wire a,
  input wire b
);

wire c = a | b;

endmodule

//-------------------------------------------------

module bar(
  input wire a
);

wire b = ~a;

endmodule
