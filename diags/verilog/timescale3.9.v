/*
 Name:     timescale3.9.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verifies that a missing time unit measurement value
           flags the appropriate error.
*/

`timescale 1 / 100 ms

module main;

initial begin
`ifdef DUMP
        $dumpfile( "timescale3.9.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
