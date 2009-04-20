/*
 Name:        score_err1.31.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/17/2009
 Purpose:     Verifies that an empty CDD file is handled properly when the score command is run against it.
*/

module main;

reg a;

initial begin
`ifdef DUMP
        $dumpfile( "score_err1.31.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1'b0;
        #10;
        $finish;
end

endmodule
