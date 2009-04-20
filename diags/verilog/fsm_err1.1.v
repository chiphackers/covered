/*
 Name:        fsm_err1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        11/20/2008
 Purpose:     Verifies that an FSM transition with an bad input state.
*/

module main;

reg clock;
reg reset;
reg head;
reg tail;
reg valid;

fsm fsm_mod( clock, reset, head, tail, valid );

endmodule
