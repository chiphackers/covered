module main;

reg	a;
reg	b, c, d, e, f, g, h;

always
  begin
   #5;
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
`ifndef VPI
	$dumpfile( "always1.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b0;
	c = 1'b0;
	d = 1'b0;
	e = 1'b0;
	f = 1'b0;
	forever #(4)
	  begin
	   b = ~b;
	   c = ~c;
	  end 
end

initial begin
	#20;
	f = 1'b1;
	#100;
	$finish;
end

endmodule
