module main(input wire verilatorclock);

reg [3:0] a, b, c;

always @(posedge verilatorclock) begin
  if( $time == 1 ) begin
    a <= 4'h0;
    b <= 4'h0;
    c <= 4'h0;
  end
  if( $time == 5 ) begin
    a <= 4'sh1 <<< 3;
    b <= 4'sh4 >>> 2;
    c <= 4'sh8 >>> 2;
  end
end

/* coverage off */
always @(posedge verilatorclock)
  if( $time == 11 ) $finish;
/* coverage on */

endmodule
