/*
 Name:        event2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        01/06/2009
 Purpose:     Verifies that multiple event wait statements report correct coverage.
*/

module main;

event a, b;
reg   x;

initial begin
	x = 1'b0;
	@(a or b)
	x = 1'b1;
end

initial begin
	#5;
	->b;
end

initial begin
`ifdef DUMP
        $dumpfile( "event2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
