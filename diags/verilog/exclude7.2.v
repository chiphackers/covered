/*
 Name:        exclude7.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/01/2008
 Purpose:     Verify that we can exclude FSM coverage information
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg        clk;
reg        reset;
reg        head;
reg        tail; 
reg        valid; 

fsma fsm (
  .clock( clk   ),
  .reset( reset ),
  .head ( head  ),
  .tail ( tail  ),
  .valid( valid )
);

initial begin
`ifdef DUMP
        $dumpfile( "exclude7.2.vcd" );
        $dumpvars( 0, main );
`endif
        reset = 1'b1;
        head  = 1'b0;
        tail  = 1'b0;
        valid = 1'b0;
        #20;
        reset = 1'b0;
        #20;
        @(posedge clk);
        head <= 1'b1;
        valid <= 1'b1;
        @(posedge clk);
        head <= 1'b0;
        tail <= 1'b1;
        @(posedge clk);
        tail  <= 1'b0;
        valid <= 1'b0;
        #20;
        $finish;
end

initial begin
        clk = 1'b0;
        forever #(2) clk = ~clk;
end

endmodule
