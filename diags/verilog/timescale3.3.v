/*
 Name:     timescale3.3.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verifies that an illegal value in the time unit value is
           appropriately flagged.
*/

`timescale 2 s / 1 s

module main;

initial begin
`ifdef DUMP
        $dumpfile( "timescale3.3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
