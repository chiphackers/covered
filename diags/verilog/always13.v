module main;

reg clk;

always #(5) clk = ~clk;

initial begin
        $dumpfile( "always13.vcd" );
        $dumpvars( 0, main );
	clk = 1'b0;
        #100;
        $finish;
end

endmodule
