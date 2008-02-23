/*
 Name:        func5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/22/2008
 Purpose:     Verifies bug fix for #1899768.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a, b;

initial begin
	a = 1'b0;
	b = foo_func( 2'b00 );
	b = foo_func( 2'b00 );
        a = b;
end

initial begin
`ifdef DUMP
        $dumpfile( "func5.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

function foo_func;
  input [1:0] sel;
  begin
    case( sel )
      2'b01 :  foo_func = 1'b0;
      2'b10 :  foo_func = 1'b1;
      default: ;
    endcase
  end
endfunction

endmodule
