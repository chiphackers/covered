module main;

foo #(10,20) bar();

initial begin
`ifndef VPI
        $dumpfile( "localparam1.3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule


module foo;

parameter  SIZEG1 = 5;
localparam SIZEL  = 5;
parameter  SIZEG2 = 5;

wire [(SIZEL-1):0]  a;
wire [(SIZEG1-1):0] b;
wire [(SIZEG2-1):0] c;

endmodule
