/*
 Name:        urandom2.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/05/2008
 Purpose:     Verify that a seeded $urandom call can be output correctly to the report file.
*/

module main;

integer   d, e;
reg [7:0] a, b;
event     c;

initial begin
	d = 1;
        e = 32'hff;
	#1;
	a = ($urandom( d ) & e);
	@(c);
	b = ($urandom( d ) & e);
end

initial begin
`ifdef DUMP
        $dumpfile( "urandom2.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
