`timescale 1 s / 1 fs

module main;

reg a;

initial begin
	$monitor( "a: %h", a );
	a = 1'b0;
	#10;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "test.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

endmodule
