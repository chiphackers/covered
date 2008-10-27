/*
 Name:        test_plusargs1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/27/2008
 Purpose:     Verifies that the $test$plusargs system function call works properly.
*/

module main;

reg a, b, c, d;

initial begin
	a = 1'b0;
	b = 1'b0;
	c = 1'b0;
	d = 1'b0;
	#5;
	if( $test$plusargs( "option1" ) )
          a = 1'b1;
	if( $test$plusargs( "option2" ) )
          b = 1'b1;
	if( $test$plusargs( "option3" ) ) begin
          c = 1'b1;
	  if( $test$plusargs( "option4" ) )
	    d = 1'b1;
        end
end

initial begin
`ifdef DUMP
        $dumpfile( "test_plusargs1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
