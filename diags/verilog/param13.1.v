module main;

reg a;

foo bar();

initial begin
	a = 1'b0;
	a = bar.b;
end

initial begin
        $dumpfile( "param13.1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule

module foo;

parameter b = 1'b1;

endmodule
