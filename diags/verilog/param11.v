module main;

parameter PARM_SIZE   = 4;
parameter PARM_OFFSET = 1;
parameter [((PARM_SIZE+PARM_OFFSET)-1):PARM_OFFSET] FOO = 11;

reg a;

initial begin
	a = FOO[3];
	#5;
	a = ~FOO[3];
end

initial begin
`ifdef DUMP
        $dumpfile( "param11.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
