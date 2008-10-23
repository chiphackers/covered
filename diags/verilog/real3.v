/*
 Name:        real3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/22/2008
 Purpose:     Verifies that the $bitstoreal and $realtobits works proplery
*/

module main;

real       a, b;
reg [63:0] c;
reg        d;

initial begin
	a = 123.456;
	d = 1'b0;
	#5;
	c = $realtobits( a );
	b = $bitstoreal( c );
	d = (a == b);
end

initial begin
`ifdef DUMP
        $dumpfile( "real3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
