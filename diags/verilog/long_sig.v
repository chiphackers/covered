module main;

wire             a;
wire  [69999:0]  b;
reg   [65536:0]  c;

assign a = 1'b0;
assign b = 70000'h0;

initial begin
`ifndef VPI
	$dumpfile( "long_sig.vcd" );
	$dumpvars( 0, main );
`endif
	c[0] = 1'b0;
	#10;
	$finish;
end

endmodule
