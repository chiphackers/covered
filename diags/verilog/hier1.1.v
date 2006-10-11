module main;

foo \this/is/some/kind/of/name[0] ();

wire [3:0] \foobar[]isthe___.neatest = ~{4{ \this/is/some/kind/of/name[0] .a}};
wire c = | \foobar[]isthe___.neatest ;

initial begin
`ifndef VPI
        $dumpfile( "hier1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

endmodule


module foo;

reg a;

initial begin
	a = 1'b0;
	#10;
	a = 1'b1;
end

endmodule
