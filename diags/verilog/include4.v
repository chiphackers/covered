/*
 Name:        include4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        01/14/2009
 Purpose:     Verifies that the missing line 'x' comes before the line containing 'c'.
*/

module main;

reg x;






initial begin
	x = 1'b0;
	#20;
	#20;
	x = ~x;
end

`include "inc_body.v"

initial begin
`ifdef DUMP
        $dumpfile( "include4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
