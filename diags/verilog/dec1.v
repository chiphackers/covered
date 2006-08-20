module main;

reg [1:0] a;

initial begin
	a = 1'b0;
	a--;
	a--;
end

initial begin
        $dumpfile( "dec1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
