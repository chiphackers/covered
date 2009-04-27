/*
 Name:        merge8.10b.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/27/2009
 Purpose:     See merge8.10a.v for details.
*/

module main;

reg  a, b;
wire c;

level2a dut( a, b, c );

initial begin
`ifdef DUMP
        $dumpfile( "merge8.10b.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1'b0;
	b = 1'b1;
        #5;
	b = 1'b0;
	#5;
        $finish;
end

endmodule
