module main;

reg	[3:0]	a;
reg	[3:0]	b;
wire	[3:0]	z;
wire		carry;
wire	[4:0]	result;

assign result = {carry, z};

adder4 add( a, b, carry, z );

initial begin
`ifdef DUMP
	$dumpfile( "instance2.vcd" );
	$dumpvars( 0, main );
`endif
	a = 4'h0;
	b = 4'h1;
	#5;
	a = 4'h8;
	b = 4'h4;
	#5;
	b = 4'h8;
	#5;
	$finish;
end

endmodule
