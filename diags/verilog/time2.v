/*
 Name:        time2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/02/2008
 Purpose:     Verify that a missed $time operation is output to the report correctly.
*/

module main;

reg   a, b;
event e;

initial begin
	a = 1'b0;
	b = 1'b0;
	@(e)
	a = ($time > 10) ? b : ~b;
end

initial begin
`ifdef DUMP
        $dumpfile( "time2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
