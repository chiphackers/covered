module main(input wire verilatorclock);

reg	c, d;

always @(posedge c or negedge c) begin d <= ~d;
end

initial begin

	c = 1'b0;
	d = 1'b0;
	
end

always @(posedge verilatorclock) begin
	if (($time%4)==0)  
		c = ~c;
end

initial begin
	if ($time==101)
	$finish;
end

endmodule
