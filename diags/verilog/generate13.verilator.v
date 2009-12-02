/*
 Name:     generate13.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     08/29/2007
 Purpose:  Verifies that Covered handles embedded hierarchy correctly within
           a generate block
*/

module main( input verilatorclock );

generate
  genvar i;
  for( i=0; i<4; i=i+1 )
    begin : U
      always @(posedge verilatorclock) begin : foo
        reg [1:0] a;
        begin : bar
          if( $time == 1 )      a <= 0;
          else if( $time == 5 ) a <= i;
        end
      end
    end
endgenerate

/* coverage off */
always @(posedge verilatorclock)
  if( $time == 11 ) $finish;
/* coverage on */

endmodule
