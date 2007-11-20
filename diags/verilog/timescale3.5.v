/*
 Name:     timescale3.5.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verifies an illegal value in the time unit slot
           is appropriately flagged.
*/

`timescale 1000 s / 1 s

module main;

initial begin
`ifndef VPI
        $dumpfile( "timescale3.5.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
