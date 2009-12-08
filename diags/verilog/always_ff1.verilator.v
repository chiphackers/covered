module main (input wire verilatorclock);

reg resetn;
reg q, d;

always_ff @(posedge verilatorclock or negedge resetn)
  if( !resetn )
    q <= 1'b0;
  else
    q <= d;

/* coverage off */
always @(posedge verilatorclock) begin
  if( $time == 1 ) begin
    resetn <= 1'b0;
    d      <= 1'b1;
  end
  if( $time == 11 ) resetn <= 1'b1;
  if( $time == 21 ) $finish;
end
/* coverage on */

endmodule
