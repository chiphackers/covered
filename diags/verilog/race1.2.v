module main;

wire       a;
wire [1:0] c;
wire [2:0] d;
reg        b;

assign a = b;
assign a = ~b;

assign c[0] = b;
assign c[1] = ~b;

assign d[1:0] = c;
assign d[2:1] = ~c;

initial begin
`ifndef VPI
	$dumpfile( "race1.2.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b0;
	#10;
	$finish;
end

endmodule
