/*
 Name:        err6.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/26/2008
 Purpose:     Verify error handling for CDD FSM arc parsing error.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg clock, reset, head, tail, valid;

fsma foo(
  .clock(clock),
  .reset(reset),
  .head(head),
  .tail(tail),
  .valid(valid)
);

initial begin
`ifdef DUMP
        $dumpfile( "err6.1.vcd" );
        $dumpvars( 0, main );
`endif
	reset = 1'b1;
        head  = 1'b0;
        tail  = 1'b0;
        valid = 1'b0;
	#10;
	reset = 1'b0;
        #10;
        $finish;
end

initial begin
	clock = 1'b0;
	forever #(1) clock = ~clock;
end

endmodule
