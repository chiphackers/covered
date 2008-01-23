module main;

wire   d;
reg    b, c;

assign d = ~a;
assign a = b & c;

initial begin
`ifdef DUMP
	$dumpfile( "implicit1.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b0;
	c = 1'b0;
	#5;
	c = 1'b1;
	#5;
	$finish;
end

endmodule
