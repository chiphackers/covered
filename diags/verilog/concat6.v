module main;

reg  [1:0] b;
wire [1:0] a = {b};

initial begin
	$dumpfile( "concat6.vcd" );
	$dumpvars( 0, main );
	b = 2'b00;
	#5;
	$finish;
end

endmodule
