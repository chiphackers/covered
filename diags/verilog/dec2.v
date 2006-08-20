module main;

integer a, i;

initial begin
        a = 0;
	for( i=0; i<32; i=i+1 )
          a--;
end

initial begin
        $dumpfile( "dec2.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
