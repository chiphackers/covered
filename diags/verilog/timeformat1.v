/*
 Name:        timeformat1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/01/2008
 Purpose:     Verifies that the $timeformat system call does not cause its logic block to be
              removed from coverage consideration.
*/

module main;

reg a;

initial begin
	a = 1'b0;
	$timeformat( 0, -3, " ms", 20 );
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "timeformat1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
