/*
 Name:        merge8.6a.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        11/13/2008
 Purpose:     Verifies that a module from a different design can be merged
              into a DUT containing that module.
*/

module main1;

reg  x, y;
wire z;

multilevel mlevel( x, y, z );

initial begin
`ifdef DUMP
        $dumpfile( "merge8.6a.vcd" );
        $dumpvars( 0, main1 );
`endif
	x = 1'b0;
	y = 1'b1;
	#5;
	y = 1'b0;
        #10;
        $finish;
end

endmodule
