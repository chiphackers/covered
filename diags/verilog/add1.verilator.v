module main(
  input wire gend_clock
);

integer i, j;

initial begin
	i = 10 + 5 + 1 + 134;
	j = 10 + 5 + -1 + -134;
end

always @(posedge gend_clock) begin
  $display( "B Time: %d", $time );
  if( $time >= 10 ) $finish;
end

endmodule
