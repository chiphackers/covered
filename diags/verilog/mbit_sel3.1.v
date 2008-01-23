module main;

wire   [48:0]  a;
reg    [127:0] b;

assign a = {b[97:96], b[118:102], b[61:35], b[101:99]};

initial begin
`ifdef DUMP
	$dumpfile( "mbit_sel3.1.vcd" );
	$dumpvars( 0, main );
`endif
	b = 128'h0123456789abcdeffedcba9876543210;
	#5;
	$finish;
end

endmodule
