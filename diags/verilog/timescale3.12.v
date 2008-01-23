/*
 Name:     timescale3.12.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verifies that a missing / flags the appropriate error.
*/

`timescale 1 s 1 ms

module main;

initial begin
`ifdef DUMP
        $dumpfile( "timescale3.12.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
