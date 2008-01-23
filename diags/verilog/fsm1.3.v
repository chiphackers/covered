module main;

reg        clk;
reg        reset;
reg        head;
reg        tail;
reg        valid;

fsm fsm (
  .clock( clk   ),
  .reset( reset ),
  .head ( head  ),
  .tail ( tail  ),
  .valid( valid )
);

initial begin
`ifdef DUMP
	$dumpfile( "fsm1.3.vcd" );
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
	#20;
	@(posedge clk);
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
