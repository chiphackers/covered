module main( input verilatorclock );

reg    a, b;

/* coverage off */
always @(posedge verilatorclock) begin
  if( $time == 1 ) begin
    a <= 1'b0;
    b <= 1'b0;
  end
  else if( $time == 5 ) begin
    a <= 1'b1;
  end
  else if( $time == 9 ) begin
    b <= 1'b1;
  end
  else if( $time == 13 ) begin
    a <= 1'b0;
  end
  else if( $time == 17 ) $finish;
end
/* coverage on */

depth1 inst0( a, b );

endmodule

module depth1( a1, b1 );

input    a1;
input    b1;

wire     c1;

assign c1 = a1 & b1;

depth2 inst1( a1, b1 );

endmodule

module depth2( a2, b2 );

input    a2;
input    b2;

wire     c2;

assign c2 = a2 ^ b2;

endmodule
