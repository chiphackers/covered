/*
 Name:        generate20.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/19/2009
 Purpose:     Verify that a generate variable can have the same name as another signal
              and handle the value correctly.
*/

module main;

generate
  for( i=0; i<16; i=i+1 ) begin : U
    reg a;
  end
endgenerate

endmodule
