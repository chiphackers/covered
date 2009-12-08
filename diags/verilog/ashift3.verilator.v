/*
 Name:        ashift3.verilator.v
 Date:        11/21/2009
 Purpose:     Verifies that an arithmetic right shift of a large value is sign-extended properly.
 Simulators:  Verilator
*/

module main(input wire verilatorclock);

reg signed [95:0] a;

always @(posedge verilatorclock) begin
  if( $time == 1 ) a <= 96'h80000000_00000000_00000000;
  if( $time == 5 ) a <= a >>> 35;
end

/* coverage off */
always @(posedge verilatorclock)
  if( $time == 11 ) $finish;
/* coverage on */

endmodule
