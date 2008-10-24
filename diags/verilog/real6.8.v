/*
 Name:        real6.8.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verifies equal for real-binary operations.
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = (3.0 == 3);
	#5;
	a = (3 == 3.00001);
end

initial begin
`ifdef DUMP
        $dumpfile( "real6.8.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
