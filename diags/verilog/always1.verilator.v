module main(input wire verilatorclock);
/* verilator lint_off WIDTH */
/* verilator lint_off COMBDLY */
reg	a;
reg	b, c, d, e, f, g, h;

always
  begin
   if ($time==5)
   a <= b;
  end

always @(posedge c) d <= ~d;

always @(negedge d) e <= ~e;

always @(e or f) g = e & f;

always @(posedge c)
  if( e & d )
    h <= 1'b0;
  else
    h <= 1'b1;

initial begin
	b = 1'b0;
	c = 1'b0;
	d = 1'b0;
	e = 1'b0;
end

always @(posedge verilatorclock) begin
  if ((verilatorclock % 4)==0)
	b = ~b;
	c = ~c;
end

initial begin
	f = 1'b0;
	if ($time==21)
	f = 1'b1;
	if ($time==121)
	$finish;
end
/* verilator lint_on COMBDLY */
/* verilator lint_on WIDTH */
endmodule
