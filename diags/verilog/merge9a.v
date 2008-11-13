/*
 Name:        merge9a.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        11/13/2008
 Purpose:     Verify that instance ranges can be merged properly.
*/

module main;

reg  [1:0] x, y;
wire [1:0] z;

multilevel mlevel [1:0] ( x, y, z );

initial begin
`ifdef DUMP
        $dumpfile( "merge9a.vcd" );
        $dumpvars( 0, main );
`endif
	x = 2'b01;
	y = 2'b11;
	#5;
	y = 2'b00;
        #10;
        $finish;
end

endmodule
