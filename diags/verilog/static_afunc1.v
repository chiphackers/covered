/*
 Name:     static_afunc1.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     07/31/2007
 Purpose:  Verify that automatic functions work properly in static function
           conditions.
*/

module main;

parameter FOO_SIZE = div2( 32 );

reg [(FOO_SIZE-1):0] foo;

initial begin
	foo = 'h0;
	#5;
	foo = 'hff;
end

initial begin
`ifndef VPI
        $dumpfile( "static_afunc1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

function automatic [31:0] div2;
  input [31:0] a;
  begin
    if( a > 0 )
      div2 = div2( a >> 1 ) + 1;
    else
      div2 = 0;
  end
endfunction

endmodule
