module main;

wire  [5:0]  a;
reg          b;
reg          c;

assign a = {3{b, c}};

initial begin
`ifdef DUMP
	$dumpfile( "expand2.vcd" );
	$dumpvars( 0, main );
`endif
	#5;
	b = 1'b0;
	#5;
	c = 1'b1;
	#5;
	b = 1'b1;
	#5;
	c = 1'b0;
	#5;
	$finish;
end

endmodule
