/*
 Name:        merge8.9a.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        11/19/2008
 Purpose:     Verifies that the middle instance in a module with more than two instances
              can be merged into a CDD that does not contain that instance in its scored list.
*/

module main;

reg  x, y;
wire z;

multilevel3 mlevel( x, y, z );

initial begin
`ifdef DUMP
        $dumpfile( "merge8.9a.vcd" );
        $dumpvars( 0, main );
`endif
	x = 1'b0;
	y = 1'b0;
	#5;
	y = 1'b1;
        #10;
        $finish;
end

endmodule
