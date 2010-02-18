/*
 Name:        mbit_sel1.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        01/21/2010
 Purpose:     Verifies interesting RHS multi-bit scenarios.
*/

module main;

reg [198:127] w;
reg [137:66]  x;
reg [135:64]  y;
reg [71:0]    z;
reg [15:0]    a, b;
reg [64:0]    c, d, e, f;
reg [73:0]    g;
reg [137:0]   h, i;
reg [128:0]   j;
reg [156:0]   k;
reg [135:0]   l;
reg [200:0]   m;

initial begin
  w = 72'hdc__cdef_89ab_4567_0123;
  x = 72'h54__4567_0123_cdef_89ab;
  y = 72'h10__0123_4567_89ab_cdef;
  z = 72'hef__fedc_ba98_7654_3210;
  a = 16'h0;
  b = 16'h0;
  c = 65'h0;
  d = 65'h0;
  e = 65'h0;
  f = 65'h0;
  g = 74'h0;
  h = 138'h0;
  j = 129'h0;
  k = 157'h0;
  l = 136'h0;
  m = 201'h0;
  #5;
  a = z[80:65];
  b = y[70:55];
  c = z[80:16];
  d = y[80:16];
  e = z[135:71];
  f = y[64:0];
  g = y[136:63];
  h = y[200:63];
  i = y[137:0];
  j = z[144:16];
  k = x[157:1];
  l = w[136:1];
  m = w[201:1];
end

initial begin
`ifdef DUMP
        $dumpfile( "mbit_sel1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
