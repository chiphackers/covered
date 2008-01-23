module main;

foo bar();

initial begin
`ifdef DUMP
	$dumpfile( "instance3.vcd" );
	$dumpvars( 0, main );
`endif
end

endmodule

module foo;

wire     a;
reg      b, c, d;

assign a = b ? 1'b0 : 1'b1;

always @(posedge c) d <= ~d;

initial begin
	b = 1'b0;
	c = 1'b0;
	d = 1'b0;
	#5;
	b = 1'b1;
	#5;
	$finish;
end

endmodule
