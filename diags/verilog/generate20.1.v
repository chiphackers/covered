/*
 Name:        generate20.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/20/2009
 Purpose:     Verifies that a generate variable is required for a generate block to perform correctly.
*/

module main;

reg i;

generate
  for( i=0; i<4; i=i+1 ) begin : U
    reg a;
  end
endgenerate

endmodule
