/*
 Name:     timescale1.5.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verifies 100 s / 100 s timescale setting.
*/

`timescale 100 s / 100 s

module main;

reg a;

initial begin
	a = 1'b0;
	#10;
	a = 1'b1;
end

initial begin
`ifndef VPI
        $dumpfile( "timescale1.5.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

endmodule
