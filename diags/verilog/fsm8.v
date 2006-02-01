module main;

parameter [1:0] STATE_IDLE = 2'b00,
                STATE_HEAD = 2'b01,
                STATE_DATA = 2'b10,
	        STATE_TAIL = 2'b11;

reg            clock;
reg            reset;
reg  [1:0]     state;
reg  [2:0]     next_state;
wire           msg_ip;
reg            head;
reg            tail;
reg            valid;

always @(posedge clock) state <= reset ? {1'b0,STATE_IDLE} : next_state;

(* covered_fsm, channel, is="state", os="next_state[1:0]" *)
always @(state or head or valid or tail)
  begin
   case( state )
     STATE_IDLE:  next_state = (valid & head) ? {1'b1,STATE_HEAD} : {1'b0,STATE_IDLE};
     STATE_HEAD:  next_state = (valid & tail) ? {1'b1,STATE_TAIL} : {1'b1,STATE_DATA};
     STATE_DATA:  next_state = (valid & tail) ? {1'b1,STATE_TAIL} : {1'b1,STATE_DATA};
     STATE_TAIL:  next_state = (valid & head) ? {1'b1,STATE_HEAD} : {1'b0,STATE_IDLE};
   endcase
  end

assign msg_ip = next_state[2];

initial begin
	$dumpfile( "fsm8.vcd" );
	$dumpvars( 0, main );
	reset = 1'b1;
	valid = 1'b0;
	head  = 1'b0;
	tail  = 1'b0;
	#20;
	reset = 1'b0;
	@(posedge clock);
        head  <= 1'b1;
        valid <= 1'b1;
	@(posedge clock);
	head  <= 1'b0;
	tail  <= 1'b1;
	@(posedge clock);
	tail  <= 1'b0;
	valid <= 1'b0;
	#20;
	$finish;
end

initial begin
	clock = 1'b0;
	forever #(1) clock = ~clock;
end

endmodule
