/*
 Name:        real5.9.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verifies not-equal for real-real operations.
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = (0.0001 != 0.00001);
	#5;
	a = (0.0001 != 0.0001);
end

initial begin
`ifdef DUMP
        $dumpfile( "real5.9.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
