module main;

wire	[31:0]	a;
reg		b;
reg		c;

assign a = b * c;

initial begin
`ifndef VPI
	$dumpfile( "mult1.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b0;
	c = 1'b0;
	#5;
	b = 1'b1;
	#5;
	c = 1'b1;
	#5;
	b = 1'b0;
	#5;
	c = 1'b0;
end

endmodule
