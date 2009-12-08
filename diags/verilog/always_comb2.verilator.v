/*
 Name:        always_comb2.verilator.v
 Date:        10/16/2009
 Purpose:     Verifies that an always_comb that never triggers is output to the report file properly.
*/

module main (input wire verilatorclock);

reg a, b, c;

always_comb
  a = b & c;

/* coverage off */
always @(posedge verilatorclock)
  if( $time == 11 ) $finish;
/* coverage on */

endmodule
