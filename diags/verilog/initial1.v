module main;

reg a;

initial begin
	a = 1'b0;
	#10;
	a = 1'b1;
end

initial begin
`ifndef VPI
        $dumpfile( "initial1.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

endmodule
