module main(input wire verilatorclock);

reg	c, d;

always @(negedge c) begin
  if( $time == 5 ) d <= 1'b0;
  d <= ~d;
end

always @(posedge verilatorclock) begin
  if( $time == 1 )
    c <= 1'b0;
  if( ($time % 4) == 1 ) 
    c <= ~c;
  if( $time == 101 )
    $finish;
end

endmodule
