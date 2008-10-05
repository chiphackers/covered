/*
 Name:        urandom_range2.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/05/2008
 Purpose:     Verify that a seeded $urandom_range call with two parameters can be output correctly to the report file.
*/

module main;

integer   d, e;
reg [7:0] a, b;
event     c;

initial begin
	d = 1;
        e = 32'hff;
	a = ($urandom_range( 1, 7 ) & e);
	@(c);
	b = ($urandom_range( 2, 8 ) & e);
end

initial begin
`ifdef DUMP
        $dumpfile( "urandom_range2.2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
