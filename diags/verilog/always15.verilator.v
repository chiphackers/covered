/*
 Name:        always15.verilator.v
 Date:        10/15/2009
 Purpose:     Verifies that always blocks with bit selects trigger properly.
*/

module main (input wire verilatorclock);

reg [3:0] a, b;

always @(a[0]) begin
  b = a;
end

/* coverage off */
always @(posedge verilatorclock) begin
  if( $time == 1 )  a <= 4'h0;
  if( $time == 5 )  a <= 4'h1;
  if( $time == 11 ) a <= 4'h2;
  if( $time == 15 ) a <= 4'h4;
  if( $time == 21 ) a <= 4'h8;
  if( $time == 31 ) $finish;
end
/* coverage on */

endmodule
