module main;

char          a = 0;
char signed   b = 0;
char unsigned c = 0;

initial begin
        $dumpfile( "char1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
