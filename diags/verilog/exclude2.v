module main;

foo_module bar( 1'b0 );

initial begin
`ifdef DUMP
        $dumpfile( "exclude2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
