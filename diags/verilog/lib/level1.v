module level1(
  input wire a,
  input  wire b,
  output wire c
);

wire d, e;

level2a l2a(a, b, d);
level2b l2b(d, e, c);

assign e = ~d;

endmodule
