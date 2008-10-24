/*
 Name:        real5.6.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verify greater-than for real-real operations.
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = (6.7 > 3.2);
	#5;
	a = (3.3 > 6.8);
end

initial begin
`ifdef DUMP
        $dumpfile( "real5.6.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
