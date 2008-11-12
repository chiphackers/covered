module level2a(
  input  wire a,
  input  wire b,
  output wire c
);

wire d, e;

level3a l3a(a, d);
level3b l3b(b, e);

assign c = d | e;

endmodule
