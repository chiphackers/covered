module main;

reg a, b, c;

initial begin
        a = 1'b0;
	#5;
        while( a == 1'b0 )
          begin
           a = b;
           #5;
          end
end

initial begin
`ifdef DUMP
        $dumpfile( "while1.1.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
        #20;
	c = 1'b1;
	#1;
        $finish;
end

endmodule
