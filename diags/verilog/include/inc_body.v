reg	b, c;
wire	a;

assign a = b & c;

initial begin
	b = 1'b0;
	c = 1'b0;
	#5;
	c = 1'b1;
	#5;
	c = 1'b0;
	#5;
	b = 1'b1;
	#5;
	c = 1'b1;
	#5;
	$finish;
end

