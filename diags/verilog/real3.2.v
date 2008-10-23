/*
 Name:        real3.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verifies that the $itor system function works properly.
*/

module main;

real a;
reg  b, c;

initial begin
	b = 1'b0;
	c = 1'b1;
	a = $itor( 123 );
	#5;
	b = (a == 123.0);
	c = (a == 123.000000001);
end

initial begin
`ifdef DUMP
        $dumpfile( "real3.2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
