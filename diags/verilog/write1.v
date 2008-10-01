/*
 Name:        write1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/30/2008
 Purpose:     Verify that the $write system call does not cause its block to be
              removed from coverage consideration.
*/

module main;

reg a;

initial begin
	$write( "Hello world!\n" );
	a = 1'b0;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "write1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
