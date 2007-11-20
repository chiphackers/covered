/*
 Name:     generate13.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     08/29/2007
 Purpose:  Verifies that Covered handles embedded hierarchy correctly within
           a generate block
*/

module main;

generate
  genvar i;
  for( i=0; i<4; i++ )
    begin : U
      initial begin : foo
        reg [1:0] a;
        begin : bar
          a = 0;
  	  #5;
          a = i;
        end
      end
    end
endgenerate

initial begin
`ifndef VPI
        $dumpfile( "generate13.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
