/*
 Name:        urandom_range3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/05/2008
 Purpose:     Verifies that the $urandom_range with the -conservative option causes the logic
	      block to be removed from coverage consideration.
*/

module main;

integer a;
event   b;

initial begin
	@(b);
	a = $urandom_range( 7, 1 );
end

initial begin
`ifdef DUMP
        $dumpfile( "urandom_range3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
