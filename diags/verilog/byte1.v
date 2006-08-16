module main;

byte          a = 0;
byte signed   b = 0;
byte unsigned c = 0;

initial begin
        $dumpfile( "byte1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
