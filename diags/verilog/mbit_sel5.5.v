/*
 Name:        mbit_sel5.5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/10/2010
 Purpose:     Verifies interesting RHS multi-bit pos scenarios.
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
reg [15:0]    n;

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
  n = 16'h0;
  #5;
  a = z[65+:16];
  b = y[55+:16];
  c = z[16+:65];
  d = y[16+:65];
  e = z[71+:65];
  f = y[0+:65];
  g = y[63+:74];
  h = y[63+:138];
  i = y[0+:138];
  j = z[16+:129];
  k = x[1+:157];
  l = w[1+:136];
  m = w[1+:201];
  n = w[1+:16];
end

initial begin
`ifdef DUMP
        $dumpfile( "mbit_sel5.5.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
