module main( input verilatorclock );

reg [2:1] a;
reg       b;

always @(a) b = a[2];

always @(posedge verilatorclock) begin
  if( $time == 1 )  a <= 2'b01;
  if( $time == 5 )  a <= 2'b10;
  if( $time == 11 ) $finish;
end

endmodule
