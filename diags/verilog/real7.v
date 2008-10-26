/*
 Name:        real7.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/25/2008
 Purpose:     Verify that a function can return a type of real.
*/

module main;

reg  a;
real b;

initial begin
	a = 1'b0;
	b = foo_func( 1'b0 );
	#5;
	a = (b == 3.4);
end

initial begin
`ifdef DUMP
        $dumpfile( "real7.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

function real foo_func;
  input sel;
  begin
    if( sel )
      foo_func = 4.6;
    else
      foo_func = 3.4;
  end
endfunction

endmodule
