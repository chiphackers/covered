module main(input wire verilatorclock);

reg       a;
reg [3:0] b, c;

always @(a)
  begin
   b = (4'h1 <<< 2);
   c = (4'h2 >>> 3);
  end

/* coverage off */
always @(posedge verilatorclock)
  if( $time == 11 ) $finish;
/* coverage on */

endmodule
