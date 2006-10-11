module main;

foo #(.SIZE(10)) bar();

initial begin
`ifndef VPI
        $dumpfile( "localparam2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule


module foo;

localparam SIZE=5;

wire [(SIZE-1):0] a;

endmodule
