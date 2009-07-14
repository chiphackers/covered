/*
 Name:        inlined_err1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        07/14/2009
 Purpose:     See script for details.
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "inlined_err1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
