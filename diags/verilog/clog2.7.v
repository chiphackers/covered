/*
 Name:        clog2.7.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        01/13/2010
 Purpose:     Verifies that a $clog in the RHS of a localparam assignment that is not
              supported due to a generation specification.
*/

module main;

localparam FOO = $clog2( 33 );

reg [FOO-1:0] a;

initial begin
  a = 0;
end

initial begin
`ifdef DUMP
        $dumpfile( "clog2.7.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
