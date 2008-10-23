/*
 Name:        real3.6.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verifies that a non-real parameter can be overridden via the -P
              option on the command-line with a real value.
*/

module main;

foo #(3.4) a( 1'b0 );

initial begin
`ifdef DUMP
        $dumpfile( "real3.6.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule

module foo(
  input wire a
);

parameter A = 4'hc;

reg b;

initial begin
	#1;
        b = a;
        #5;
        b = (A == 3.4);
end

endmodule
