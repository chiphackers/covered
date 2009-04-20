/*
 Name:        signed5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/06/2009
 Purpose:     Verifies that the $signed system call works properly.
*/

module main;

reg [3:0] a, b;
reg       x, y;

initial begin
	a = 4'h3;
	b = 4'h4;
	x = 1'b0;
	y = 1'b0;
	#5;
	if( (a - b) < 0 )
	  x = 1'b1;
	if( $signed( a - b ) < 0 )
	  y = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "signed5.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
