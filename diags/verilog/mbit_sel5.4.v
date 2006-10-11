module main;

foo #(.b( 0)) f0();
foo #(.b( 4)) f1();
foo #(.b( 8)) f2();
foo #(.b(12)) f3();

initial begin
`ifndef VPI
        $dumpfile( "mbit_sel5.4.vcd" );
        $dumpvars( 0, main );
`endif
	#10;
        $finish;
end

endmodule


module foo;

parameter              incr = 4;
parameter [(incr-1):0] b    = 0;
parameter [15:0]       a    = 16'h8421;
parameter [(incr-1):0] c    = a[b+:incr];

wire [(c-1):0] bar = 0;

endmodule
