/*
 Name:     timescale3.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verify timescale error condition.
*/

`timescale 100 ms / 1 s

module main;

initial begin
`ifndef VPI
        $dumpfile( "timescale3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
