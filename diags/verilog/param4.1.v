module main;

parameter vec = { 2'b01, 2'b10 };

wire [3:0] a;
reg  [3:0] b;

assign a = b & vec;

initial begin
	$dumpfile( "param4.1.vcd" );
	$dumpvars( 0, main );
	b = 4'hd;
	#5;
	b = 4'h1;
	#10;
	$finish;
end

endmodule
