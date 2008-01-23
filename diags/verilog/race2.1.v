module main;

reg  clock;
reg  a, b;

always @(posedge clock) a = 1'b0;
always @(posedge clock) b <= 1'b1;

initial begin
`ifdef DUMP
	$dumpfile( "race2.1.vcd" );
	$dumpvars( 0, main );
`endif
	clock = 1'b0;
	#5;
	clock = 1'b1;
	#5;
	clock = 1'b0;
	#10;
	$finish;
end

endmodule
