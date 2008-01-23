/*
 Name:     timescale3.4.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verifies an illegal value in the time precision slot is
           appropriately flagged.
*/

`timescale 10 s / 2 s

module main;

initial begin
`ifdef DUMP
        $dumpfile( "timescale3.4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
