module main;

reg a;

initial begin
	a = 1'b0;
	a++;
	a++;
end

initial begin
`ifndef VPI
        $dumpfile( "inc1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
