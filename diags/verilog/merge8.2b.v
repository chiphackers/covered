/*
 Name:        merge8.1b.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        11/02/2008
 Purpose:     See merge8.1a.v for details.
*/

module main;

reg  [3:0] a, b;
wire [3:0] z;
wire       c;

adder4 adder( a, b, c, z );

initial begin
`ifdef DUMP
        $dumpfile( "merge8.2b.vcd" );
        $dumpvars( 0, main );
`endif
	a = 4'h0;
	b = 4'h0;
	#5;
	a = 4'h0;
	b = 4'h1;
        #10;
        $finish;
end

endmodule
