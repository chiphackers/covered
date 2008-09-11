/*
 Name:        always_comb2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/11/2008
 Purpose:     Verifies that an always_comb that never triggers is output to the report file properly.
*/

module main;

reg a, b, c;

always_comb
  a = b & c;

initial begin
`ifdef DUMP
        $dumpfile( "always_comb2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
