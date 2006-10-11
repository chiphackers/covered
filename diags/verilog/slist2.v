module main;

reg a, b, c;

always @*
   a = b & c;

initial begin
`ifndef VPI
        $dumpfile( "slist2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
