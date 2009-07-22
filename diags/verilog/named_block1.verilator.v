module main(input verilatorclock);

reg a, b;

initial begin : foobar
	a = 1'b0;
end

initial begin : barfoo
	b = 1'b1;
end

always @(posedge verilatorclock) begin
  if( $time == 9 ) begin
	begin : fooey
	 reg c, d;
	 c <= a & b;
	 d <= a | b;
	end
	begin : barry
         reg e, f;
	 e <= a ^ b;
	 f <= a ~^ b;
	end
  end
end

/* coverage off */
always @(posedge verilatorclock) begin
  if( $time == 99 ) $finish;
end
/* coverage on */

endmodule
