/*
 Name:        clog2.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        01/12/2010
 Purpose:     Verifies clog2 system operation with a wide input.
*/

module main;

integer a, b, c, d;

initial begin
	a = 0;
	b = 0;
	c = 0;
	d = 0;
	#5;
	a = $clog2( 128'h1 );
	b = $clog2( 128'h20_00000000 );
        c = $clog2( 128'h40000_00000000_00000000 );
        d = $clog2( 128'h80000000_00000000_00000000_00000000 );
end

initial begin
`ifdef DUMP
        $dumpfile( "clog2.2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
