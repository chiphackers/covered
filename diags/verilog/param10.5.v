module main;

foo #(2)       a();
foo #(3,4)     b();
foo #(.car(6)) c();

initial begin
`ifdef DUMP
        $dumpfile( "param10.5.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule


module foo;

parameter bar = 1;

wire [(bar-1):0] b;
reg  [31:0]      c;

initial begin : foo_begin
	parameter car = 5;
	reg [(car-1):0] a;
	a = 1'b0;
end

initial begin
        c = 0;
        c = bar;
end

endmodule
