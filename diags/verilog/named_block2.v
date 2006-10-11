module main;

initial begin :foo
end

initial begin
`ifndef VPI
        $dumpfile( "named_block2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
