module main;

parameter STATE_IDLE = 2'b00,
          STATE_HEAD = 2'b01,
          STATE_DATA = 2'b10,
          STATE_TAIL = 2'b11;

reg        clk;
reg        reset;
reg        head;
reg        tail;
reg        valid;
reg [1:0]  state;
reg [1:0]  next_state;

always @(posedge clk) state <= reset ? STATE_IDLE : next_state;

(* covered_fsm, channel, is="state", os="next_state",
                         trans="STATE_HEAD->STATE_DATA",
                         trans="STATE_DATA->STATE_TAIL",
                         trans="STATE_DATA->STATE_DATA",
                         trans="STATE_TAIL->STATE_HEAD" *)
always @(state or head or valid or tail)
  begin
   case( state )
     STATE_IDLE:  next_state = (valid & head) ? STATE_HEAD : STATE_IDLE;
     STATE_HEAD:  next_state = (valid & tail) ? STATE_TAIL : STATE_DATA;
     STATE_DATA:  next_state = (valid & tail) ? STATE_TAIL : STATE_DATA;
     STATE_TAIL:  next_state = (valid & head) ? STATE_HEAD : STATE_IDLE;
   endcase
  end

initial begin
	$dumpfile( "fsm7.3.vcd" );
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
