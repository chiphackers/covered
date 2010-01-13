/*
 Name:        clog2.3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        01/12/2010
 Purpose:     Verifies $clog2 system function used as a constant function.
*/

module main;

reg [$clog2(32)-1:0] a;
reg [$clog2(33)-1:0] b;

initial begin
  a = 0;
  b = 0;
end

initial begin
`ifdef DUMP
        $dumpfile( "clog2.3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
