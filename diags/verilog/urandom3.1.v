/*
 Name:        urandom3.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/05/2008
 Purpose:     Verify that a $urandom function call with the -conservative option set removes
              the logic block from coverage consideration.
*/

module main;

integer a, c;
event   b;

initial begin
	c = 1;
	@(b);
	a = $urandom( c );
end

initial begin
`ifdef DUMP
        $dumpfile( "urandom3.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
