/*
 Name:     timescale3.12.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     11/25/2006
 Purpose:  Verifies that a missing / flags the appropriate error.
*/

`timescale 1 s 1 ms

module main;

initial begin
`ifndef VPI
        $dumpfile( "timescale3.12.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
