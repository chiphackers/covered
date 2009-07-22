module main(input verilatorclock);

reg  a;
reg  b, c;

always @(a)
  begin : foo
   b =  a;
   c = ~a;
  end

/* coverage off */
always @(posedge verilatorclock) begin
  if( $time == 1 ) a = 1'b0;
  if( $time == 3 ) a = 1'b1;
  if( $time == 5 ) $finish;
end
/* coverage on */

endmodule
