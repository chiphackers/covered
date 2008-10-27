/*
 Name:        value_plusargs3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/27/2008
 Purpose:     Verifies that the $value$plusargs system function call works for decimal numbers.
*/

module main;

reg        a, b, c, d;
reg [63:0] x;
reg [31:0] y;
reg        z;

initial begin
	a = 1'b0;
        b = 1'b0;
	c = 1'b0;
	d = 1'b0;
	x = 64'h0;
	y = 32'h0;
	z = 1'b0;
	#5;
	if( $value$plusargs( "optionD=%d", x ) )
          a = 1'b1;
	if( $value$plusargs( "optionD=%d", y ) )
	  b = 1'b1;
        else if( $value$plusargs( "optiond=%d", z ) )
          c = 1'b1;
	if( $value$plusargs( "optiond=%d", z ) )
	  d = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "value_plusargs3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
