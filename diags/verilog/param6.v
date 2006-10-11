module main;

parameter SIZE0 = 1;
parameter SIZE1 = 2;

reg    [7:0]       b;
wire   [SIZE1-1:0] a;

assign a = b[SIZE1+SIZE0:SIZE0+1];

initial begin
`ifndef VPI
	$dumpfile( "param6.vcd" );
	$dumpvars( 0, main );
`endif
	b = 8'hac;
	#5;
	$finish;
end

endmodule
