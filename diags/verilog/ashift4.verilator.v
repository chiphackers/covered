/*
 Name:        ashift4.verilator.v
 Date:        11/30/2009
 Purpose:     Verify that the correct thing happens when the arithmetic right shift value is X.
 Simulators:  VERILATOR
*/

module main( input verilatorclock );

reg signed [39:0] a;
reg [31:0] b;

always @(posedge verilatorclock) begin
  if( $time == 1 ) a <= 40'h8000000000;
  if( $time == 5 ) begin
    b <= 32'd1;
    a <= a >>> b;
  end
  if( $time == 11 ) begin
    b[20] <= 1'bx;
    a     <= a >>> b;
  end
  if( $time == 15 ) $finish;
end

endmodule
