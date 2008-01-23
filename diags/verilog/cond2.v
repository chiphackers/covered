module main;

wire	a;
reg	b;
reg	c;
reg	d;

assign a = b ? c : d;

initial begin
`ifdef DUMP
	$dumpfile( "cond2.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b1;
	c = 1'b0;
	d = 1'b1;
	#5;
	c = 1'b1;
	#5;
end

endmodule
