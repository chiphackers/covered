/*
 Name:        real2.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/16/2008
 Purpose:     Verify that a real number can be assigned from the VCD file.
*/

module main;

real       a, b;
reg [31:0] c;

initial begin
	c = 0;
	#5;
	c = a + b;
end

initial begin
`ifdef DUMP
        $dumpfile( "real2.1.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1.1;
	b = 3.7;
        #10;
        $finish;
end

endmodule
