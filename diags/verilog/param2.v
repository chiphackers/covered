module main;

wire    a;
reg     b;

parameter value0 = 2'b01;
parameter value1 = 1'b0;

assign a = b ? value0[1] : value1;

initial begin
	$dumpfile( "param2.vcd" );
	$dumpvars( 0, main );
	b = 1'b0;
	#5;
	b = 1'b1;
	#5;
	$finish;
end

endmodule
