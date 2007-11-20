/*
 Name:     timescale3.6.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verifies an illegal value in the time precision slot
           is appropriately flagged.
*/

`timescale 1 s / 1000 us

module main;

initial begin
`ifndef VPI
        $dumpfile( "timescale3.6.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
