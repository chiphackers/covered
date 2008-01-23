module main;

parameter [0:7] FOOBAR = 8'h17;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = FOOBAR[0];
end

initial begin
`ifdef DUMP
        $dumpfile( "endian3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
