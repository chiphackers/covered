module main;

wire [2:0] a;
reg  [1:0] b;
reg        c;

assign a[2:1] = b;
assign a[0]   = c;

initial begin
`ifndef VPI
	$dumpfile( "mbit_sel2.vcd" );
	$dumpvars( 0, main );
`endif
	b = 2'b00;
	c = 1'b1;
	#5;
	b = 2'b01;
	#5;
	b = 2'b10;
	c = 1'b0;
	#5;
	$finish;
end

endmodule
