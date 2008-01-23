module main;

reg [31:0] a;
reg        b;

initial begin
        a = 4;
        b = 1'b0;
	do
          begin
           #5;
           if( !a[1] )
             a = a + 2;
           else
             a = a + 1;
          end
        while( a < 10 );
	b = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "do_while1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
