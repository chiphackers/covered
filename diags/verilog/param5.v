module main;

parameter dude = 2;

reg    [(dude-1):0]      a;
wire   [((dude*2)-1):0]  b;

assign b = {a, ~a};

initial begin
`ifdef DUMP
	$dumpfile( "param5.vcd" );
	$dumpvars( 0, main );
`endif
	a = 2'b00;
	#5;
	a = 2'b10;
	#5;
	a = 2'b01;
	#5;
	a = 2'b11;
	#5;
	$finish;
end

endmodule
