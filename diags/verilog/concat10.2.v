/*
 Name:        concat10.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        01/12/2010
 Purpose:     Verifies fix for bug 2929948.
*/

module main;

reg [31:0] a, b, c, d;
reg [31:0] w, x, y, z;

initial begin
  a = 0;
  b = 0;
  c = 0;
  d = 0;
  w = 3;
  x = 2;
  y = 1;
  z = 0;
  #5;
  {a[0], b, c, d} = {w[0], x, y, z};
end

initial begin
`ifdef DUMP
        $dumpfile( "concat10.2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
