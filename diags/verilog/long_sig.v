module main;

wire            a;
wire  [1999:0]  b;
reg   [1024:0]  c;

assign a = 1'b0;
assign b = 2000'h0;

initial begin
	$dumpfile( "long_sig.vcd" );
	$dumpvars( 0, main );
	c[0] = 1'b0;
	#10;
	$finish;
end

endmodule
