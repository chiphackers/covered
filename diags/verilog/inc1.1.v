module main;

reg a;

initial begin
	a = 1'b0;
	a++;
	@(a);
	a++;
end

initial begin
        $dumpfile( "inc1.1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
