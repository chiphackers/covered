/*
 Name:        real6.5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verifies less-than-or-equal for real-binary operations.
*/

module main;

reg a, b;

initial begin
	a = 1'b0;
	b = 1'b0;
	#5;
	a = (4.999995 <= 5);
	b = (5.0 <= 5);
	#5;
	a = (5 <= 4.999995);
end

initial begin
`ifdef DUMP
        $dumpfile( "real6.5.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
