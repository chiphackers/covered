module main;

reg a, b;

initial begin
        a = 1'b0;
        b = 1'b0;
        wait( a == 1'b0 );
        b = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "wait1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
