/*
 Name:     timescale2.5.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verifies multiple timescale specifications within the design when
           top module timescale is 1 s / 1 ns.
*/

`timescale 1 s / 1 ns

module main;

ts_module tsm();

initial begin
`ifndef VPI
        $dumpfile( "timescale2.5.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
