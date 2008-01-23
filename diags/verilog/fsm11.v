/*
 Name:     fsm11.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     08/06/2007
 Purpose:  Verify usage of defined values in FSM attribute list.
*/

`define ZERO 1'b0
`define ONE  1'b1

module main;

reg a, b;

(* covered_fsm, foo, is="a", os="b",
   trans="`ONE->`ZERO",
   trans="1'b0->1'b1" *)
always @(a)
  case( a )
    `ZERO :  b = `ONE;
    `ONE  :  b = `ZERO;
  endcase

initial begin
`ifdef DUMP
        $dumpfile( "fsm11.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1'b0;
        #10;
        $finish;
end

endmodule
