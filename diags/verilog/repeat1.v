module main;

reg a;

initial begin
        a = 1'b0;
	repeat( 2 )
          a = ~a;
end

initial begin
`ifdef DUMP
        $dumpfile( "repeat1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
