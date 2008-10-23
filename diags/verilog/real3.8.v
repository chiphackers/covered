/*
 Name:        real3.8.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verifies that a negative real number can be specified to override
              a parameter value from the command-line.
*/

module main;

foo #(-3.4) a( 1'b0 );

initial begin
`ifdef DUMP
        $dumpfile( "real3.8.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule

module foo(
  input wire a
);

parameter A = 3.5;

reg  b;
real c;

initial begin
	#1;
	b = a;
	c = A;
	#5;
	b = (c == -3.4);
end 

endmodule
