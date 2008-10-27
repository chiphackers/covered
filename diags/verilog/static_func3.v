/*
 Name:        static_func3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/25/2008
 Purpose:     Verifies that static functions returning real values work properly.
*/

module main;

parameter A = foo_func( 1'b0 );

reg a;

initial begin
	a = 1'b0;
	#5;
	a = (A == 3.4);
end

initial begin
`ifdef DUMP
        $dumpfile( "static_func3.vcd" );
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
