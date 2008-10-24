/*
 Name:        timescale4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/24/2008
 Purpose:     Verifies that real delays work properly when timescale is 100 s / 100 s
*/

`timescale 100 s / 100 s

module main;

reg a;

initial begin
	a = 1'b0;
	#(2.123456789987654);
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "timescale4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
