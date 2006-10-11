module main;

reg a;

initial begin
	a = 0;
	assert( a == 0 );
	#5;
	a = 1;
end

initial begin
`ifndef VPI
        $dumpfile( "assert1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
