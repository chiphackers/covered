module main;

longint          a = 0;
longint signed   b = 0;
longint unsigned c = 0;

initial begin
        $dumpfile( "longint1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
