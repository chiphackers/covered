module main;

wire a;

foobar bar( a );

initial begin
`ifndef VPI
	$dumpfile( "race1.vcd" );
	$dumpvars( 0, main );
`endif
	#10;
	$finish;
end

endmodule


module foobar ( a );

input a;

assign a = 1'b0;

endmodule
