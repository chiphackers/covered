/*
 Name:     signed4.1.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/03/2006
 Purpose:  Verifies that parser can correctly parse and handle
           signed functions and signed function inputs.
*/

module main;

initial begin
	$display( "%d", foo_func( -1 ) );
end

initial begin
`ifdef DUMP
        $dumpfile( "signed4.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

function signed [31:0] foo_func;
  input signed [31:0] in;
  begin
    foo_func = in - 1;
  end
endfunction

endmodule
