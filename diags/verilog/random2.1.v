/*
 Name:        random2.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/02/2008
 Purpose:     Verify that a seeded $random call can be output correctly to the report file.
*/

module main;

integer   d, e;
reg [7:0] a, b;
event     c;

initial begin
	d = 1;
        e = 32'hff;
	#1;
	a = ($random( d ) & e);
	@(c);
	b = ($random( d ) & e);
end

initial begin
`ifdef DUMP
        $dumpfile( "random2.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
