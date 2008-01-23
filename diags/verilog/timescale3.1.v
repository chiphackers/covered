/*
 Name:     timescale3.1.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/25/2006
 Purpose:  Verifiying that error condition is reported when an illegal
           timescale measurement is specified.
*/

`timescale 1 gs / 1 s

module main;

initial begin
`ifdef DUMP
        $dumpfile( "timescale3.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
