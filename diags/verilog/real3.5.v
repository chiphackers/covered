/*
 Name:        real3.5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verifies that a parameter storing a real value that is overridden with a non-real
              value gets treated as a non-real value.
*/

module main;

foo #(4'h1) a( 1'b0 );

initial begin
`ifdef DUMP
        $dumpfile( "real3.5.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule

module foo(
  input wire a
);

parameter A = 3.4;

reg b;

initial begin
	#1;
	b = ~a;
	#5;
	b = (A == 3.4);
end

endmodule
