`define ST_STOP     0
`define ST_GO       1
`define ST_SLOW     2

module main;

reg        clk;
reg        go;
wire [1:0] state;

fsma fsm1( clk, go, state );
fsmb fsm2( clk, go );

wire error = (state == `ST_STOP) && (state == `ST_GO) && (state == `ST_SLOW);

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
output [1:0] state;

reg [1:0] next_state;
reg [1:0] state;

initial begin
	state = `ST_STOP;
end

always @(posedge clk) state <= next_state;

always @(state or go)
  case( state )
    `ST_STOP :  next_state = go ? `ST_GO : `ST_STOP;
    `ST_GO   :  next_state = go ? `ST_GO : `ST_SLOW;
    `ST_SLOW :  next_state = `ST_STOP;
  endcase

endmodule

module fsmb( clk, go );

input     clk;
input     go;
     
reg [1:0] next_state;
reg [1:0] state;  
     
initial begin
        state = `ST_STOP;
end     
        
always @(posedge clk) state <= next_state;
        
(* covered_fsm, lights, is="state", os="next_state",
                        trans="0->1",
                        trans="1->2",
                        trans="2->0" *)
always @(state or go)
  case( state )
    `ST_STOP :  next_state = go ? `ST_GO : `ST_STOP;
    `ST_GO   :  next_state = go ? `ST_GO : `ST_SLOW;
    `ST_SLOW :  next_state = `ST_STOP;
  endcase
        
endmodule
