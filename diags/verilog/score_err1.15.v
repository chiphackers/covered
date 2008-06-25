/*
 Name:        score_err1.15.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/24/2008
 Purpose:     Verify that two -vcd options are flagged as an error to the user.
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
        $dumpfile( "score_err1.15.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
