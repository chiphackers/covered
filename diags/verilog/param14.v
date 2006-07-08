module main;

parameter FOO = 0;

reg [FOO:0] a, b;

initial begin
        $dumpfile( "param14.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
