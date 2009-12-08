/*
 Name:        always_latch2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/11/2008
 Purpose:     Verifies that an always_latch that never triggers is output to the report file correctly.
*/

module main(input wire verilatorclock);
/* verilator lint_off COMBDLY */
reg a, b, c;

always_latch
  a <= b & c;

/* coverage off */
always @(posedge verilatorclock)
  if( $time == 11 ) $finish;
/* coverage on */
/* verilator lint_on COMBDLY */
endmodule
