/*
 Name:        real5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verify that real-real addition operation works properly.
*/

module main;

real a;
reg  b;

initial begin
	a = 1.4 + 2.6;
	b = 1'b0;
	#5;
	b = (a == 4.0);
end

initial begin
`ifdef DUMP
        $dumpfile( "real5.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
