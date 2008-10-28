/*
 Name:        value_plusargs6.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/27/2008
 Purpose:     Verifies that the $value$plusargs system function call works for real numbers.
*/

module main;

reg  a, b;
real x, y;

initial begin
	a = 1'b0;
        b = 1'b0;
	#5;
	if( $value$plusargs( "optionF=%f", x ) && ($bitstoreal( fabs( $realtobits(x), $realtobits(123.456) ) ) < 0.00001) )
          a = 1'b1;
        else if( $value$plusargs( "optionf=%f", y ) && ($bitstoreal( fabs( $realtobits(y), $realtobits(123.456) ) ) < 0.00001) )
          b = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "value_plusargs6.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

function [63:0] fabs;
  input [63:0] a;
  input [63:0] b;
  real x, y;
  begin
    x = $bitstoreal( a );
    y = $bitstoreal( b );
    if( x > y )
      fabs = $realtobits( x - y );
    else
      fabs = $realtobits( y - x );
  end
endfunction

endmodule
