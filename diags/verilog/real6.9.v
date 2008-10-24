/*
 Name:        real6.9.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verifies not-equal for real-binary operations.
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = (3.000001 != 3);
	#5;
	a = (3.0 != 3);
end

initial begin
`ifdef DUMP
        $dumpfile( "real6.9.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
