/*
 Name:        score_ts1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/24/2008
 Purpose:     Exercises the use of the -ts option to the score command.
*/

module main;

reg clock;

initial begin
	clock = 1'b0;
	repeat( 20 ) begin
	  #5;
	  clock = ~clock;
	end
	$finish;
end

initial begin
`ifdef DUMP
        $dumpfile( "score_ts1.vcd" );
        $dumpvars( 0, main );
`endif
end

endmodule
