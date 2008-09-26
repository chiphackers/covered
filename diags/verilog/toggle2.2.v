/*
 Name:        toggle2.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/26/2008
 Purpose:     Verify that a 0->1 transition is not detected when a Z value is inbetween the 0 and 1 values.
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = 1'bz;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "toggle2.2.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
