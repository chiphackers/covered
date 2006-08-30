module main;

parameter BAR0 = 1'b0;
parameter BAR1 = 1'b1;

reg m[0:1];

initial begin : foo
        m[0] = BAR0;
	m[1] = BAR1;
end

initial begin
        $dumpfile( "mem2.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
