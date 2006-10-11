module main;

wire [4:0] a;
reg        b;

assign a = {5{b}};

initial begin
`ifndef VPI
	$dumpfile( "expand1.vcd" );
	$dumpvars( 0, main );
`endif
	#5;
	b = 1'b0;
	#5;
	b = 1'bz;
	#5;
	$finish;
end

endmodule
