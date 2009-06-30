/*
 Name:        static3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/30/2009
 Purpose:     Verifies that static ternary expressions work.
*/

module main;

parameter FOOBAR = 0;
parameter A2 = 2;
parameter A3 = 3;
parameter A4 = 4;
parameter A5 = 5;
parameter A6 = 6;
parameter A7 = 7;
parameter A8 = 8;

reg [(4?0:1):0]   a;
reg [(0?2:1):0]   b;
reg [(4?A2:3):0]  c;
reg [(4?3:A4):0]  d;
reg [(4?A4:A5):0] e;
reg [(0?6:A5):0]  f;
reg [(0?7:A6):0]  g;
reg [(0?A8:A7):0] h;

initial begin
	a = 'h0;
	b = 'h0;
        c = 'h0;
        d = 'h0;
        e = 'h0;
        f = 'h0;
        g = 'h0;
        h = 'h0;
	#5;
	a = 'hff;
	b = 'hff;
	c = 'hff;
	d = 'hff;
	e = 'hff;
	f = 'hff;
	g = 'hff;
	h = 'hff;
end

initial begin
`ifdef DUMP
        $dumpfile( "static3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
