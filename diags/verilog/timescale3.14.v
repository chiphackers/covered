/*
 Name:     timescale3.14.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     11/25/2006
 Purpose:  Verifies that missing timescale information flags appropriate error.
*/

`timescale

module main;

initial begin
`ifndef VPI
        $dumpfile( "timescale3.14.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
