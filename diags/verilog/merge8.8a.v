/*
 Name:        merge8.8a.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        11/13/2008
 Purpose:     Verifies that a root tree can be merged into a subroot tree.
*/

module main;

reg  x, y;
wire z;

multilevel mlevel( x, y, z );

initial begin
`ifdef DUMP
        $dumpfile( "merge8.8a.vcd" );
        $dumpvars( 0, main );
`endif
	x = 1'b1;
	y = 1'b1;
	#5;
	y = 1'b0;
        #10;
        $finish;
end

endmodule
