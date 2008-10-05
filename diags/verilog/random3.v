/*
 Name:        random3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/05/2008
 Purpose:     Verify that a random function call with the -conservative option set removes
              the logic block from coverage consideration.
*/

module main;

integer a;
event   b;

initial begin
	@(b);
	a = $random;
end

initial begin
`ifdef DUMP
        $dumpfile( "random3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
