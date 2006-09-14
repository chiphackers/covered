module main;

reg	clock, reset, a, b, sel;

wire	z;

initial begin
	$dumpfile( "ifelse1.vcd" );
	$dumpvars( 0, main );
	clock = 1'b0;
	forever #(5) clock = ~clock;
end

initial begin
	a     = 1'b0;
	b     = 1'b1;
	sel   = 1'b0;
	reset = 1'b1;
	#10;
	a     = 1'b1;
	#10;
	b     = 1'b0;
	#10;
	reset = 1'b0;
	#7;
	sel   = 1'b1;
	#21;
	$finish;
end

mux switch( clock, reset, a, b, sel, z );

endmodule
