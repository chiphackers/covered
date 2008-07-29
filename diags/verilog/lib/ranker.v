module ranker (
  input [127:0] a,
  input [127:0] b,
  input [127:0] c,
  input [  2:0] d
);

reg [127:0] z1;

always @( a )
  case( d[2:0] )
    3'h0 :  z1 = a;
    3'h1 :  z1 = b;
    3'h2 :  z1 = c;
    3'h3 :  z1 = ~a;
    3'h4 :  z1 = a & b & c;
    3'h5 :  z1 = a | b | c;
    3'h6 :  z1 = a ^ b & c;
    3'h7 :  z1 = (a & b) | (b & c) | (a & c);
  endcase

endmodule
