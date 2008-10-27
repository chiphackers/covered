/*
 Name:        afunc2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/25/2008
 Purpose:     Verifies that automatic functions using real values work properly.
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = (div2_real( $realtobits( 32.0 ) ) == 5.0);
end

initial begin
`ifdef DUMP
        $dumpfile( "afunc2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

function automatic real div2_real;
  input [63:0] a;
  real real_a;
  begin
    div2_real = 0.0;
    real_a = $bitstoreal( a );
    if( (real_a - 0.1) > 1.0 )
      div2_real = div2_real( $realtobits( real_a / 2.0 ) ) + 1.0;
  end
endfunction

endmodule
