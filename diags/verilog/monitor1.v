/*
 Name:        monitor1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/01/2008
 Purpose:     Verifies that the $monitor system call does not cause its logic block to be
              removed from coverage consideration.
*/

module main;

reg a;

initial begin
 	$monitor( a );
	a = 1'b0;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "monitor1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
