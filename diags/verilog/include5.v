/*
 Name:        include5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/08/2009
 Purpose:     Verifies that included modules are parsed correctly.
*/

`include "adder1.v"

module main;

reg  a, b;
wire c, z;

adder1 foo( a, b, c, z );

initial begin
`ifdef DUMP
        $dumpfile( "include5.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1'b0;
	b = 1'b1;
        #10;
        $finish;
end

endmodule
