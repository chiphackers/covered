/*
 Name:        fwrite1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/30/2008
 Purpose:     Verify that an $fwrite system call does not cause its logic block to be
              removed from coverage consideration.
*/

module main;

reg a;

initial begin
	$fwrite( 1, "Hello world\n" );
	a = 1'b0;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "fwrite1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
