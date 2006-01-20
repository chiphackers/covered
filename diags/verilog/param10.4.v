module main;

foo a();

initial begin
        $dumpfile( "param10.4.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule


module foo;

parameter bar = 2;

wire [(bar-1):0] b;
reg  [31:0]      c;

initial begin : foo_begin
	parameter bar = 5;
	reg [(bar-1):0] a;
	a = 1'b0;
end

initial begin
        c = 0;
        c = bar;
end

endmodule
