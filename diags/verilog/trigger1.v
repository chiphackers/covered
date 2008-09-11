/*
 Name:        trigger1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/11/2008
 Purpose:     Verifies that a trigger line is output to the report properly.
*/

module main;

event a, b;

initial begin
	-> a;
	@(b);
	-> a;
end

initial begin
`ifdef DUMP
        $dumpfile( "trigger1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
