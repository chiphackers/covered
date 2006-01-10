module main;

reg a, b, c;

always @*
   a = b & c;

initial begin
        $dumpfile( "slist2.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
