/*
 Name:        merge8.11b.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/28/2009
 Purpose:     See merge8.11a.v for details.
*/

module main;

wire a;
reg  b, c;

dut_and_proc dut( a, b, c );

initial begin
`ifdef DUMP
        $dumpfile( "merge8.11b.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b1;
	c = 1'b1;
	#5;
	c = 1'b0;
        #10;
        $finish;
end

endmodule
