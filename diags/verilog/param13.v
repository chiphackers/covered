module main;

parameter foo = 1'b1;

reg a;

initial begin : bar
	a = 1'b0;
	a = foo;
end

initial begin
`ifdef DUMP
        $dumpfile( "param13.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
