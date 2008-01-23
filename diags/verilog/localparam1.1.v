module main;

foo #(10) bar();

initial begin
`ifdef DUMP
        $dumpfile( "localparam1.1.vcd" );
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
