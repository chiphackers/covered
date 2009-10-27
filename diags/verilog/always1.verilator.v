module main(input wire verilatorclock);
/* verilator lint_off WIDTH */
// /* verilator lint_off COMBDLY */
reg	a;
/* verilator lint_off MULTIDRIVEN */
reg	b, c, d, e, f, g, h;
/* verilator lint_on MULTIDRIVEN */

always @(posedge c) d <= ~d;

always @(negedge d) e <= ~e;

always @(e or f) g = e & f;

always @(posedge c)
  if( e & d )
    h <= 1'b0;
  else
    h <= 1'b1;

always @(posedge verilatorclock) begin
  if( $time == 1 ) begin
    b <= 1'b0;
    c <= 1'b0;
    d <= 1'b0;
    e <= 1'b0;
  end
  if( ($time % 4) == 1 ) begin
    b <= ~b;
    c <= ~c;
  end
end

always @(posedge verilatorclock) begin
  if( $time == 1 )  f <= 1'b0;
  if( $time == 21 ) f <= 1'b1;
  if( $time == 121) $finish;
end

/* verilator lint_on WIDTH */
// /* verilator lint_on COMBDLY */
endmodule
