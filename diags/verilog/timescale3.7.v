/*
 Name:     timescale3.7.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     11/25/2006
 Purpose:  Verifies that a bad time unit string is appropriately flagged.
*/

`timescale foo / 1 s

module main;

initial begin
`ifndef VPI
        $dumpfile( "timescale3.7.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
