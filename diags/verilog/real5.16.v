/*
 Name:        real5.16.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/29/2008
 Purpose:     Verify that the divide-assign feature works for real-real numbers.
*/

module main;

real a;
reg  b;

initial begin
	a = 123.456;
	b = 1'b0;
	#5;
	a /= 2.0;
	b = ($bitstoreal( fabs( $realtobits( 61.728 ), $realtobits( a ) ) ) < 0.00001);
end

initial begin
`ifdef DUMP
        $dumpfile( "real5.16.vcd" );
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

