/*
 Name:        clog2.6.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        01/13/2010
 Purpose:     Verifies that a $clog in the LHS of a parameter assignment that is not
              supported due to a generation specification.
*/

module main;

parameter FOO = $clog2( 33 );

reg [FOO-1:0] a;

initial begin
  a = 0;
end

initial begin
`ifdef DUMP
        $dumpfile( "clog2.6.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
