module main;

wire a;

foobar bar( a );

initial begin
	$dumpfile( "race1.1.vcd" );
	$dumpvars( 0, main );
	#10;
	$finish;
end

endmodule


module foobar ( a );

input a;

wire b;
reg c;

initial c = 1'b0;

assign a =  b;
assign a = ~b;
assign b = c;

endmodule
