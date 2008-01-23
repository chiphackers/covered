module main;

parameter SIZE0 = 1;
parameter SIZE1 = 2;

reg    [SIZE1+SIZE0:0] b;
wire   [SIZE1-1:0]     a;

assign a = b[SIZE1+SIZE0:SIZE0+1];

initial begin
`ifdef DUMP
	$dumpfile( "param6.1.vcd" );
	$dumpvars( 0, main );
`endif
	b = 4'hc;
	#5;
	$finish;
end

endmodule
