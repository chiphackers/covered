module main;

reg     a;
integer i;

initial begin
        i = 0;
 	a = 1'b0;
	if( --i == 0 ) a = 1'b1;
end

initial begin
        $dumpfile( "dec3.1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
