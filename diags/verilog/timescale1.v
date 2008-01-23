/*
 Name:     timescale1.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verifies 1 s / 1 s timescale setting.
*/

`timescale 1 s / 1 s

module main;

reg a;

initial begin
	a = 1'b0;
	#10;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "timescale1.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

endmodule
