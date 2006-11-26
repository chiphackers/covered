/*
 Name:     timescale3.8.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     11/25/2006
 Purpose:  Verifies that a bad value in the time precision slot
           will be appropriately flagged.
*/

`timescale 1 s / bar

module main;

initial begin
`ifndef VPI
        $dumpfile( "timescale3.8.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
