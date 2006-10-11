module main;

int          a = 0;
int signed   b = 0;
int unsigned c = 0;

initial begin
`ifndef VPI
        $dumpfile( "int1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
