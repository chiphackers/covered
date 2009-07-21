module main(
  input wire gend_clock
);

integer i, j;

initial begin
	i = 10 + 5 + 1 + 134;
	j = 10 + 5 + -1 + -134;
end

/* coverage off */
always @(posedge gend_clock) begin
  if( $time >= 10 ) $finish;
end
/* coverage on */

endmodule
