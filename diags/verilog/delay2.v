module main;

reg   a, b;
reg   clk;

always @(posedge clk)
  begin
   #(0.5);
   a <= b;
  end

initial begin
`ifndef VPI
	$dumpfile( "delay2.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b0;
	#5;
	b = 1'b1;
	#6;
	$finish;
end

initial begin
	clk = 1'b0;
	forever #(2.5) clk = ~clk;
end

endmodule
