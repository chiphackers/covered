module main;

reg rst, clk;

always #(5) clk = rst ? 1'b0 : ~clk;

initial begin
        $dumpfile( "always13.vcd" );
        $dumpvars( 0, main );
        rst = 1'b1;
	#10;
	rst = 1'b0;
        #100;
        $finish;
end

endmodule
