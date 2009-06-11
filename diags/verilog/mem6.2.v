/*
 Name:        mem6.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/11/2009
 Purpose:     Verifies that a memory element written within a concatenation
              assigned within a non-blocking assignment identifies the correct
              memory coverage.
*/

module main;

reg [3:0] a[0:1];
reg       b;

initial begin
	a[0]     <= 4'h0;
	#5;
	{a[0],b} <= 5'h2;
end

initial begin
`ifdef DUMP
        $dumpfile( "mem6.2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
