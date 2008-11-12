/*
 Name:        merge8.5a.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        11/12/2008
 Purpose:     Verifies that a mix of non-contiguous and overlapping hierarchies can be merged properly.
*/

module main;

reg  x, y;
wire z;

multilevel mlevel(x, y, z);

initial begin
`ifdef DUMP
        $dumpfile( "merge8.5a.vcd" );
        $dumpvars( 0, main );
`endif
	x = 1'b0;
	y = 1'b0;
	#5;
	x = 1'b1;
        #10;
        $finish;
end

endmodule
