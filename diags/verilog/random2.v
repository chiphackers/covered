/*
 Name:        random2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/02/2008
 Purpose:     Verifies that $random without parameters outputs properly in a report.
*/

module main;

integer   d;
reg [7:0] a, b;
event     c;

initial begin
	d = 32'hff;
	a = $random & d;
	@(c)
	b = $random & d;
end

initial begin
`ifdef DUMP
        $dumpfile( "random2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
