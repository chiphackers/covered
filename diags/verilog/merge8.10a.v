/*
 Name:        merge8.10a.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/27/2009
 Purpose:     Verifies that a lower-level module testbench can be merged into a full testbench.
*/

module main;

reg  a, b;
wire c;

multilevel dut( a, b, c );

initial begin
`ifdef DUMP
        $dumpfile( "merge8.10a.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1'b0;
	b = 1'b0;
        #5;
	b = 1'b1;
	#5;
        $finish;
end

endmodule
