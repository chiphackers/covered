/*
 Name:        toggle2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/26/2008
 Purpose:     Verifies that toggle 0->1 works properly when an X value is introduced between the 0 and 1 values.
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = 1'bx;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "toggle2.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
