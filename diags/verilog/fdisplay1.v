/*
 Name:        fdisplay1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/30/2008
 Purpose:     Verify that the $fdisplay system call does not cause its block to be removed
              from coverage consideration.
*/

module main;

reg a;

initial begin
	$fdisplay( 1, "Hello world!" );
	a = 1'b0;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "fdisplay1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
