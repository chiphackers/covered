/*
 Name:        toggle2.3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/26/2008
 Purpose:     Verify that a 1->0 toggle is not hit when a Z value is inserted between the 1 and 0 values.
*/

module main;

reg a;

initial begin
	a = 1'b1;
	#5;
	a = 1'bz;
	#5;
	a = 1'b0;
end

initial begin
`ifdef DUMP
        $dumpfile( "toggle2.3.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
