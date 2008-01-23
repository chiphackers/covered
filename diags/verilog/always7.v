module main;

event a;

always @(a) #5;

initial begin
`ifdef DUMP
	$dumpfile( "always7.vcd" );
	$dumpvars( 0, main );
`endif
	->a;
	#10;
	->a;
	#10;
	$finish;
end

endmodule
