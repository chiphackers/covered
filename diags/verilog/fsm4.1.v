module main;

parameter STATE_IDLE = 16'h0001,
          STATE_HEAD = 16'h0010,
          STATE_DATA = 16'h0100,
          STATE_TAIL = 16'h1000;

reg         clk;
reg         reset;
reg         head;
reg         tail;
reg         valid;
reg [15:0]  state;

always @(posedge clk)
  if( reset )
    state = STATE_IDLE;
  else
   case( state )
     STATE_IDLE:  state <= (valid & head) ? STATE_HEAD : STATE_IDLE;
     STATE_HEAD:  state <= (valid & tail) ? STATE_TAIL : STATE_DATA;
     STATE_DATA:  state <= (valid & tail) ? STATE_TAIL : STATE_DATA;
     STATE_TAIL:  state <= (valid & head) ? STATE_HEAD : STATE_IDLE;
   endcase

initial begin
	$dumpfile( "fsm4.1.vcd" );
	$dumpvars( 0, main );
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
	tail <= 1'b0;
	head <= 1'b1;
	@(posedge clk);
	tail <= 1'b1;
        head <= 1'b0;
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
