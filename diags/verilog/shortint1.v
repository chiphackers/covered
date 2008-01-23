module main;

shortint          a = 0;
shortint signed   b = 0;
shortint unsigned c = 0;

initial begin
`ifdef DUMP
        $dumpfile( "shortint1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
