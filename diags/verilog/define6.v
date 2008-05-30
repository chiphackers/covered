/*
 Name:        define6.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/29/2008
 Purpose:     Verifies that continuation sequence in define removes the newline so that
              line numbers are reported properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

`define zero  2'b00
`define one   2'b01
`define two   2'b10
`define three 2'b11
`define plus  2 + \
              3

module main;

reg [1:0] a, b;

always @*
  case( a )
    `zero  :
       b = `plus;
    `one   :
       b = `zero;
    `two   :
       b = `one;
    `three :
       b = `two;
  endcase

initial begin
`ifdef DUMP
        $dumpfile( "define6.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
