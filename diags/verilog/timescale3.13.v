/*
 Name:     timescale3.13.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verifies that missing timescale unit information
           flags appropriate error.
*/

`timescale / 1 s

module main;

initial begin
`ifdef DUMP
        $dumpfile( "timescale3.13.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
