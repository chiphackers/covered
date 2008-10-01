/*
 Name:        display1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/30/2008
 Purpose:     Verify that the $display system call does not exclude its block from coverage.
*/

module main;

reg a;

initial begin
	$display( "Hello world" );
	a = 1'b0;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "display1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
