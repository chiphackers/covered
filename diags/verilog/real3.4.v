/*
 Name:        real3.4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verifies that a non-real parameter can be overridden with a real value and
              be treated as a real value.
*/

module main;

foo #(3.4) a( 1'b0 );

initial begin
`ifdef DUMP
        $dumpfile( "real3.4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule

module foo(
  input wire a
);

parameter A = 4'h0;

real b;
reg  c;

initial begin
	#1;
	c = a;
	#5;
	c = (A == 3.4);
end

endmodule
