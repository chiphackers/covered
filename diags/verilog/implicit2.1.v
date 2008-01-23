module main;

real     r;
realtime rt;

initial begin
`ifdef DUMP
	$dumpfile( "implicit2.1.vcd" );
	$dumpvars( 0, main );
`endif
	r  = 3;
	rt = 4;
	#5;
	r  = 30;
	rt = 40;
	#5;
	$finish;
end

endmodule
