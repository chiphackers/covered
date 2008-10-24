/*
 Name:        real5.5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verify less-than-or-equal for real-real operations.
*/

module main;

reg a, b;

initial begin
	a = 1'b0;
	b = 1'b0;
	#5;
	a = (3.4 <= 3.5);
	b = (3.4 <= 3.4);
	#5;
	a = (3.5 <= 3.4);
end

initial begin
`ifdef DUMP
        $dumpfile( "real5.5.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
