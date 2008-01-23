module main;

reg  clock, x;
reg  a, b;

always @(posedge clock or x) a  =  x;
always @(posedge clock or x) b <= ~x;

initial begin
`ifdef DUMP
	$dumpfile( "race2.3.vcd" );
	$dumpvars( 0, main );
`endif
	x     = 1'b0;
        clock = 1'b0;
	#5;
	clock = 1'b1;
	#5;
	clock = 1'b0;
	#10;
	$finish;
end

endmodule
