module main;

reg        clk;
reg        reset;
reg        head1, head2;
reg        tail1, tail2;
reg        valid1, valid2;

fsm fsm1 (
  .clock( clk    ),
  .reset( reset  ),
  .head ( head1  ),
  .tail ( tail1  ),
  .valid( valid1 )
);

fsm fsm2 (
  .clock( clk    ),
  .reset( reset  ),
  .head ( head2  ),
  .tail ( tail2  ),
  .valid( valid2 )
);

initial begin
	$dumpfile( "fsm10.vcd" );
	$dumpvars( 0, main );
        reset  = 1'b1;
	head1  = 1'b0;
        tail1  = 1'b0;
        valid1 = 1'b0;
	head2  = 1'b0;
        tail2  = 1'b0;
        valid2 = 1'b0;
	#20;
	reset = 1'b0;
	#20;
	@(posedge clk);
        head1 <= 1'b1;
	valid1 <= 1'b1;
	@(posedge clk);
        head1 <= 1'b0;
	tail1 <= 1'b1;
	@(posedge clk);
	tail1  <= 1'b0;
	valid1 <= 1'b0;
	#20;
	$finish;
end

initial begin
	clk = 1'b0;
        forever #(2) clk = ~clk;
end

endmodule
