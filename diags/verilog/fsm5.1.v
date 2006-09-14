module main;

parameter STATE_IDLE = 2'b00,
          STATE_HEAD = 2'b01,
          STATE_DATA = 2'b10,
	  STATE_TAIL = 2'b11;

reg            clock;
reg            reset;
reg  [2:0]     state;
reg  [1:0]     next_state;
wire           msg_ip;
reg            head;
reg            tail;
reg            valid;

always @(posedge clock) state <= reset ? {STATE_IDLE,1'b0} : {next_state,(next_state != STATE_IDLE)};

always @(state or head or valid or tail)
  begin
   case( state[2:1] )
     STATE_IDLE:  next_state = (valid & head) ? STATE_HEAD : STATE_IDLE;
     STATE_HEAD:  next_state = (valid & tail) ? STATE_TAIL : STATE_DATA;
     STATE_DATA:  next_state = (valid & tail) ? STATE_TAIL : STATE_DATA;
     STATE_TAIL:  next_state = (valid & head) ? STATE_HEAD : STATE_IDLE;
   endcase
  end

assign msg_ip = state[0];

initial begin
	$dumpfile( "fsm5.1.vcd" );
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
