/*
 Name:        repeat3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/11/2008
 Purpose:     Verifies that when a repeat statement is not fully covered it is output to the
              report file correctly.
*/

module main;

reg a, b, c, d;

initial begin
	a = 1'b0;
	b = 1'b1;
	c = 1'b1;
	d = 1'b0;
	#5;
	d = repeat( 3 ) @(posedge a) b & c;
end

initial begin
`ifdef DUMP
        $dumpfile( "repeat3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
