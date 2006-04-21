module main;

parameter foo = 1'b0;

reg a;

initial begin : bar
	a = foo;
end

initial begin
        $dumpfile( "param13.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
