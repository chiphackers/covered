module main;

wire [3:0]   a;
reg          b0;
reg  [1:0]   b1;
reg          b2;

assign a = {b2, b1, b0};

initial begin
	$dumpfile( "concat1.vcd" );
	$dumpvars( 0, main );
	b0 = 1'b1;
	b1 = 2'h2;
	b2 = 1'b0;
	#5;
	b1 = 2'h1;
	b2 = 1'b1;
	#5;
	$finish;
end

endmodule
