/*
 Name:        exclude9.8.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/23/2008
 Purpose:     Verifies that excluding coverage for a default case
              with begin...end block
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [1:0] a;
reg       b;

always @*
  case( a )
    2'b00   :  b = 1'b0;
    2'b10   :  b = 1'b1;
    // coverage off
    default :
      begin
        b = 1'bx;
        $display( "HERE!" );
      end
    // coverage on
  endcase

initial begin
`ifdef DUMP
        $dumpfile( "exclude9.9.vcd" );
        $dumpvars( 0, main );
`endif
	a = 2'b00;
        #10;
        $finish;
end

endmodule
