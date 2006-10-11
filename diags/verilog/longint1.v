module main;

longint          a = 0;
longint signed   b = 0;
longint unsigned c = 0;

initial begin
`ifndef VPI
        $dumpfile( "longint1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
