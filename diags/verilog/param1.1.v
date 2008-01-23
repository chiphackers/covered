module main;

wire    a;
reg     b;

parameter value0 = 1'b0;
parameter value1 = value0 | 1'b1;

assign a = b ? value0 : value1;

initial begin
`ifdef DUMP
	$dumpfile( "param1.1.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b0;
	#5;
	b = 1'b1;
	#5;
	$finish;
end

endmodule
