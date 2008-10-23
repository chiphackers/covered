/*
 Name:        real3.3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verify that real parameters work properly.
*/

module main;

parameter A = 123.456;

reg  a;
real b;

initial begin
	a = 1'b0;
	b = A;
	#5;
	a = (A == b);
end

initial begin
`ifdef DUMP
        $dumpfile( "real3.3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
