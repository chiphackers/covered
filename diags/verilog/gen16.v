module main;

reg a, b, c;

always @(a, b, c)
  a = b & c;

endmodule
