/*
 Name:        real5.4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verify less-than operation for real-real expressions.
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = (3.2 < 4.7);
	#5;
	a = (4.8 < 3.1);
end

initial begin
`ifdef DUMP
        $dumpfile( "real5.4.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
