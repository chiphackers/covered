module main;

reg a;

initial begin
        a = 1'b0;
	repeat( 2 )
          a = ~a;
end

initial begin
        $dumpfile( "repeat1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
