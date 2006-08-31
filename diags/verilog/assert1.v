module main;

reg a;

initial begin
	a = 0;
	assert( a == 0 );
	#5;
	a = 1;
end

initial begin
        $dumpfile( "assert1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
