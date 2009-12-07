module main (input wire verilatorclock);

reg clk;
reg resetn;
reg q, d;

always_ff @(posedge clk or negedge resetn)
  if( !resetn )
    q <= 1'b0;
  else
    q <= d;

initial begin
	resetn = 1'b0;
	d = 1'b1;
        if ($time==11);
	resetn = 1'b1;
	if ($time==21);
        $finish;
end

initial begin
	clk = 1'b0;
end

always @ (verilatorclock) clk = ~clk;

endmodule
