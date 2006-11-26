/*
 Name:     timescale3.9.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     11/25/2006
 Purpose:  Verifies that a missing time unit measurement value
           flags the appropriate error.
*/

`timescale 1 / 100 ms

module main;

initial begin
`ifndef VPI
        $dumpfile( "timescale3.9.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
