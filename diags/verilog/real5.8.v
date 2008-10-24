/*
 Name:        real5.8.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verifies equal for real-real operations.
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = (4.567 == 4.567);
	#5;
	a = (4.567 == 4.568);
end

initial begin
`ifdef DUMP
        $dumpfile( "real5.8.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
