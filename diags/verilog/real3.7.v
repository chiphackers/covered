/*
 Name:        real3.7.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verifies that signed negative numbers can be assigned to real values and retain signedness.
*/

module main;

real    a;
integer b;
reg     c;

initial begin
	a = -123.456;
	b = a;
	c = 1'b0;
	#5;
	c = (b == -123);
end

initial begin
`ifdef DUMP
        $dumpfile( "real3.7.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
