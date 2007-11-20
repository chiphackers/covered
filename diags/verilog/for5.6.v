/*
 Name:     for5.6.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/11/2007
 Purpose:  Verifies local char declaration in for loops.
*/

module main;

initial begin : foo
	reg [1:0] a;
        reg [1:0] i;
	i = 0;
	#5;
	for( char unsigned i=0; i<4; i++ ) begin : for1
	  a = i;
        end
	i = a;
end

initial begin
`ifndef VPI
        $dumpfile( "for5.6.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
