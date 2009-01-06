/*
 Name:        endian4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        01/05/2009
 Purpose:     Verifies that a big endian signal is handled correctly when read from a dumpfile.
*/

module main;

reg [2:4] a;
reg       b;

initial begin
	b = 1'b1;
	#10;
	b = a[4];
	#1;
	b = a[2];
end

initial begin
`ifdef DUMP
        $dumpfile( "endian4.vcd" );
        $dumpvars( 0, main );
`endif
	a = 3'h0;
	#5;
	a = 3'h6;
        #10;
        $finish;
end

endmodule
