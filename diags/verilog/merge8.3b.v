/*
 Name:        merge8.3b.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        11/12/2008
 Purpose:     See merge8.3a.v for details.
*/

module main;

reg  x, y;
wire z;

multilevel mlevel(x, y, z);

initial begin
`ifdef DUMP
        $dumpfile( "merge8.3b.vcd" );
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
