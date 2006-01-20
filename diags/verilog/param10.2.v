module main;

foo a();

initial begin
        $dumpfile( "param10.2.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule


module foo;

initial begin : foo_begin
	parameter bar = 5;
	reg [(bar-1):0] a;
	a = 1'b0;
end

endmodule
