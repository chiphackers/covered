module multilevel(
  input  wire a,
  input  wire b,
  output wire c
);

wire d;

level1 l1(a, b, c);

assign d = ~c;

endmodule
