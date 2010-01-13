/*
 Name:        clog2.4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        01/12/2010
 Purpose:     Verifies a static $clog2 expression with a non-static value.
*/

module main;

parameter FOO = 32;
parameter BAR = 63;

reg [$clog2(FOO)-1:0] a;
reg [$clog2(BAR)-1:0] b;

initial begin
  a = 0;
  b = 0;
end

initial begin
`ifdef DUMP
        $dumpfile( "clog2.4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
