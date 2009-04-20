/*
 Name:        unsigned1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/06/2009
 Purpose:     Verify that the $unsigned system call works properly.
*/

module main;

reg signed [3:0] a, b;
reg              x, y;

initial begin
	a = 4'h3;
	b = 4'h4;
	x = 1'b0;
	y = 1'b0;
	#5;
	if( (a - b) < 0 )
          x = 1'b1;
	if( $unsigned( a - b ) < 0 )
          y = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "unsigned1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
