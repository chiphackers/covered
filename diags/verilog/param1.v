module main;

wire    a;
reg     b;

parameter value0 = 1'b0;
parameter value1 = 1'b0;

assign a = b ? value0 : value1;

initial begin
	$dumpfile( "param1.vcd" );
	$dumpvars( 0, main );
	b = 1'b0;
	#5;
	b = 1'b1;
	#5;
	$finish;
end

endmodule
