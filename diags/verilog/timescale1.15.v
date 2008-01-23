/*
 Name:     timescale1.15.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verifies 1 s / 1 us timescale setting.
*/

`timescale 1 s / 1 us

module main;

reg a;

initial begin
	a = 1'b0;
	#10;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "timescale1.15.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

endmodule
