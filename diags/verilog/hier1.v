module main;

wire [1:0] a = foo.a;

bar foo();

initial begin
`ifndef VPI
	$dumpfile( "hier1.vcd" );
	$dumpvars( 0, main );
`endif
end

endmodule


module bar;

reg [1:0] a;

initial begin
	a = 2'b00;
end

initial begin
	#5;
	$finish;
end

endmodule
