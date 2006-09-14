module main;

reg  b, c, d;

wire a = b ? 
         c : d;

initial begin
	$dumpfile( "long_exp3.vcd" );
	$dumpvars( 0, main );
	b = 1'b0;
	c = 1'b0;
	d = 1'b0;
	#10;
	$finish;
end

endmodule
