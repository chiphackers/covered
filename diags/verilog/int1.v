module main;

int          a = 0;
int signed   b = 0;
int unsigned c = 0;

initial begin
        $dumpfile( "int1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
