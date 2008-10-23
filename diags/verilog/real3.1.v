/*
 Name:        real3.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/22/2008
 Purpose:     Verify that the $rtoi system function causes the value to be truncated, but that
              an implicit assignment causes the value to be rounded.
*/

module main;

real       a;
reg [63:0] b, c;
reg        d;

initial begin
	b = 64'h0;
	c = 64'h0;
	d = 1'b0;
	#5;
	a = 123.5;
	b = $rtoi( a );
	c = a;
	d = (b != c);
end

initial begin
`ifdef DUMP
        $dumpfile( "real3.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
