/*
 Name:        static_func4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/30/2009
 Purpose:     Verifies that a function used statically can also be used during
              runtime.
*/

module main;

reg [find_max(5,3)-1:0] a;

initial begin
	a = 0;
	#5;
	a = find_max(2,4);
end

initial begin
`ifdef DUMP
        $dumpfile( "static_func4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

function [31:0] find_max;

  input [31:0] a;
  input [31:0] b;

  begin
    if( a > b )
      find_max = a;
    else
      find_max = b;
  end

endfunction

endmodule
