/*
 Name:        merge11.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        11/14/2008
 Purpose:     See script for details.
*/

module main;

reg  x, y;
wire z;

multilevel mlevel( x, y, z );

initial begin
`ifdef DUMP
        $dumpfile( "merge11.vcd" );
        $dumpvars( 0, main );
`endif
	x = 1'b1;
	y = 1'b0;
	#5;
	y = 1'b1;
        #10;
        $finish;
end

endmodule
