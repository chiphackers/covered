module multilevel3(
  input  wire a,
  input  wire b,
  output wire c
);

wire c1, c2, c3;

level1 l1a(  a,  b, c1 );
level1 l1b(  a, ~b, c2 );
level1 l1c( ~a,  b, c3 );

assign c = c1 ^ c2 ^ c3;

endmodule
