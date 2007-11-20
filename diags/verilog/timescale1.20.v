/*
 Name:     timescale1.20.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verifies that timescale information without spaces
           is handled properly.
*/

`timescale 1s/100ms

module main;

reg a;

initial begin
	a = 1'b0;
	#10;
	a = 1'b1;
end

initial begin
`ifndef VPI
        $dumpfile( "timescale1.20.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

endmodule
