/*
 Name:        sbit_sel3.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        03/29/2008
 Purpose:     Verifies that function call in LHS single-bit part select
              works properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [31:0] a, b;

initial begin
	a = 0;
	b = 1;
	#5;
	a[foo(b)] = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "sbit_sel3.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

function [31:0] foo;
  input [31:0] x;
  begin
    foo = x << 2;
  end
endfunction

endmodule
