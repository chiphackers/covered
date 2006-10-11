module main;

foo #(.p1(10),.p2(20)) bar();

initial begin
`ifndef VPI
        $dumpfile( "param9.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule


module foo;

parameter p1 = 5;
parameter p2 = 5;

wire [(p1-1):0] a;
wire [(p2-1):0] b;

endmodule
