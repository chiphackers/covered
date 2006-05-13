`define ST_STOP  3'b001
`define ST_GO    3'b010
`define ST_SLOW  3'b100

module main;

reg        clk;
reg        go;
wire [2:0] state;

fsma fsm1( clk, go, state );
fsmb fsm2( clk, go );

wire error = (state[0] & state[1]) || (state[0] & state[2]) || (state[1] & state[2]) || (state == 3'b000);

initial begin
	$dumpfile( "example.vcd" );
	$dumpvars( 0, main );
	go = 1'b0;
	repeat( 10 ) @(posedge clk);
	go = 1'b1;
	#10;
	$finish;
end

initial begin
	clk = 1'b0;
	forever #(1) clk = ~clk;
end

endmodule

module fsma( clk, go, state );

input        clk;
input        go;
output [2:0] state;

reg [2:0] next_state;
reg [2:0] state;

initial begin
	state = `ST_SLOW;
end

always @(posedge clk) state <= next_state;

(* covered_fsm, lights, is="state", os="next_state" *)
always @(state or go)
  case( state )
    `ST_STOP :  next_state = go ? `ST_GO : `ST_STOP;
    `ST_GO   :  next_state = go ? `ST_GO : `ST_SLOW;
    `ST_SLOW :  next_state = `ST_STOP;
  endcase

assert_one_hot #(.width(3)) zzz_check_state ( clk, 1'b1, state );

endmodule

module fsmb( clk, go );

input     clk;
input     go;

reg [2:0] next_state;
reg [2:0] state;

initial begin
        state = `ST_STOP;
end

always @(posedge clk) state <= next_state;

(* covered_fsm, lights, is="state", os="next_state",
                        trans="3'b001->3'b010",
                        trans="3'b010->3'b100",
                        trans="3'b100->3'b001" *)
always @(state or go)
  case( state )
    `ST_STOP :  next_state = go ? `ST_GO : `ST_STOP;
    `ST_GO   :  next_state = go ? `ST_GO : `ST_SLOW;
    `ST_SLOW :  next_state = `ST_STOP;
  endcase

assert_one_hot #(.width(3)) zzz_check_state ( clk, 1'b1, state );

endmodule
