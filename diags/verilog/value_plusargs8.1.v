/*
 Name:        value_plusargs8.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/27/2008
 Purpose:     Verifies that the $value$plusargs system function call works for non-double-quoted strings.
*/

module main;

reg              a, b;
reg [(20*8)-1:0] x, y;

initial begin
	a = 1'b0;
        b = 1'b0;
	x = 160'h0;
	y = 160'h0;
	#5;
	if( $value$plusargs( "optionS2=%s", x ) )
          a = 1'b1;
        else if( $value$plusargs( "options=%s", y ) )
          b = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "value_plusargs8.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
