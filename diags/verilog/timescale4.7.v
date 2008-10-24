/*
 Name:        timescale4.7.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/24/2008
 Purpose:     Verifies that real delays work properly when timescale is 100 s / 10 us
*/

`timescale 100 s / 10 us

module main;

reg a, b;

initial begin
	a = 1'b0;
	b = 1'b0;
	#(2.123_456_789_987_654);
	b = 1'b1;
	a = ($time == 21234568);
end

initial begin
`ifdef DUMP
        $dumpfile( "timescale4.7.vcd" );
        $dumpvars( 0, main );
`endif
        #(10);
        $finish;
end

endmodule
