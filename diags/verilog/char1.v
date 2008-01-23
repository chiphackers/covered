module main;

char          a = 0;
char signed   b = 0;
char unsigned c = 0;

initial begin
`ifdef DUMP
        $dumpfile( "char1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
