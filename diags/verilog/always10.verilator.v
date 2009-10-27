module main (input wire verilatorclock);
/* verilator lint_off COMBDLY */
/* verilator lint_off MULTIDRIVEN */
reg  a, b, c;

always @(posedge verilatorclock)
  if( a )
    b <= c;

// racecheck off
always @(negedge verilatorclock) begin
  if( $time == 20 ) a <= 1'b1;
end

always @(posedge verilatorclock) begin
  $display( "Time: %t", $time );
  if( $time == 1 ) begin
    a <= 1'b0;
    c <= 1'b1;	
  end
  if( $time == 21 ) a <= 1'b0;
  if( $time == 41 ) $finish;
end
// racecheck on

/* verilator lint_on MULTIDRIVEN */
/* verilator lint_on COMBDLY */

endmodule
