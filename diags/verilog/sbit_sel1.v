module main;

reg     [4:1]    z;
wire             a, b, c, d;

assign a = z[1];
assign b = z[2];
assign c = z[3];
assign d = z[4];

initial begin
	$dumpfile( "sbit_sel1.vcd" );
	$dumpvars( 0, main );
	z = 4'h0;
	#5;
	z = 4'h1;
	#5;
	z = 4'h4;
	#5;
	z = 4'h8;
	#5;
	z = 4'h0;
	#5;
	$finish;
end

endmodule
