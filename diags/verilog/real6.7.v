/*
 Name:        real6.7.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verifies greater-than-or-equal for real-binary operations.
*/

module main;

reg a, b;

initial begin
	a = 1'b0;
	b = 1'b0;
	#5;
	a = (3.000001 >= 3);
	b = (3.0 >= 3);
	#5;
	a = (3 >= 3.000001);
end

initial begin
`ifdef DUMP
        $dumpfile( "real6.7.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
