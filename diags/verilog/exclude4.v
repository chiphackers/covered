module main;

foo_module bar( 1'b0 );

initial begin
        $dumpfile( "exclude4.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
