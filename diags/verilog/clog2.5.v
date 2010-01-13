/*
 Name:        clog2.5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        01/13/2010
 Purpose:     
*/

module main;

parameter FOO = $clog2( 33 );

reg [FOO-1:0] a;

initial begin
  a = 0;
end

initial begin
`ifdef DUMP
        $dumpfile( "clog2.5.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
