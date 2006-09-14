module main;

foo #(3) a();

initial begin
        $dumpfile( "param10.3.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule


module foo;

parameter hoo = 1;

wire [(hoo-1):0] b;

initial begin : foo_begin
	parameter bar = 5;
	reg [(bar-1):0] a;
	a = 1'b0;
end

endmodule
