/*
 Name:        monitor1.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/01/2008
 Purpose:     Verifies that the $monitoron and $monitoroff system calls do not cause its logic block to be
              removed from coverage consideration.
*/

module main;

reg a;

initial begin
        $monitor( a );
        $monitoroff;
	a = 1'b0;
	#5;
	$monitoron;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "monitor1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
