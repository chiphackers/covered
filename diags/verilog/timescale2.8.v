/*
 Name:     timescale2.8.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verifies multiple timescale specifications within the design when
           top module timescale is 100 ms / 1 ms.
*/

`timescale 100 ms / 1 ms

module main;

ts_module tsm();

initial begin
`ifndef VPI
        $dumpfile( "timescale2.8.vcd" );
        $dumpvars( 0, main );
`endif
        #9;
        $finish;
end

endmodule
