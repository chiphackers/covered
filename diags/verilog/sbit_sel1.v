module main;

reg	[2:0]	 sel;
reg     [4:1]    z;
wire             a, b, c, d, e;

assign a = z[1];
assign b = z[2];
assign c = z[3];
assign d = z[4];
assign e = z[sel+1];

initial begin
`ifdef DUMP
	$dumpfile( "sbit_sel1.vcd" );
	$dumpvars( 0, main );
`endif
	sel = 3'h2;
	z = 4'h0;
	#5;
	z = 4'h1;
	#5;
	z = 4'h4;
	#5;
        sel = 3'h0;
	z = 4'h8;
	#5;
	z = 4'h0;
	#5;
	$finish;
end

endmodule
