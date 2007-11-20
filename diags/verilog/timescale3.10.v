/*
 Name:     timescale3.10.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verifies that a missing precision measurement flags the
           appropriate error.
*/

`timescale 1 s / 100

module main;

initial begin
`ifndef VPI
        $dumpfile( "timescale3.10.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
