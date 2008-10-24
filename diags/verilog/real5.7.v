/*
 Name:        real5.7.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verifies greater-than-or-equal for real-real operations.
*/

module main;

reg a, b;

initial begin
	a = 1'b0;
	b = 1'b0;
	#5;
	a = (10.24 >= 10.23);
	b = (10.24 >= 10.24);
	#5;
	a = (10.23 >= 10.24);
end

initial begin
`ifdef DUMP
        $dumpfile( "real5.7.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
