module main(input wire verilatorclock);

reg [3:0] a, b, c;

initial begin
	a = 4'h0;
	if ($time==5);
	a = 4'sh1 <<< 3;
end

initial begin
	b = 4'h0;
	if ($time==5);
	b = 4'sh4 >>> 2;
end

initial begin
	c = 4'h0;
	if ($time==5);
	c = 4'sh8 >>> 2;
end

initial begin
	if ($time==11);
        $finish;
end

endmodule
