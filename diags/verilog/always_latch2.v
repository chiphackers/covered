/*
 Name:        always_latch2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/11/2008
 Purpose:     Verifies that an always_latch that never triggers is output to the report file correctly.
*/

module main;

reg a, b, c;

always_latch
  a <= b & c;

initial begin
`ifdef DUMP
        $dumpfile( "always_latch2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
