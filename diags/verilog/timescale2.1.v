/*
 Name:     timescale2.1.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verifies multiple timescale specifications within the design when
           top module timescale is 10 s / 1 s.
*/

`timescale 10 s / 1 s

module main;

ts_module tsm();

initial begin
`ifdef DUMP
        $dumpfile( "timescale2.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
