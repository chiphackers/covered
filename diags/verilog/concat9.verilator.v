/*
 Name:        concat9.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        08/04/2009
 Purpose:     Verify that a concatenation with a sized decimal value is handled properly.
*/

module main( input verilatorclock );

wire [11:0] a;
reg         b;

assign a = b ? {1'b0,10'd44,1'b1} : {1'b1,10'd42,1'b1};

/* coverage off */
always @(posedge verilatorclock) begin
  if( $time == 1 ) b = 1'b0;
  else if( $time == 11 ) $finish;
end
/* coverage on */

endmodule
