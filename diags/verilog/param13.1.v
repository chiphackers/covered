module main;

reg a;

foo bar();

initial begin
	a = 1'b0;
	a = bar.b;
end

initial begin
`ifndef VPI
        $dumpfile( "param13.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule

module foo;

parameter b = 1'b1;

endmodule
