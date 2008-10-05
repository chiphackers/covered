/*
 Name:        urandom2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/05/2008
 Purpose:     Verifies that $urandom without parameters outputs properly in a report.
*/

module main;

integer   d;
reg [7:0] a, b;
event     c;

initial begin
	d = 32'hff;
	a = $urandom & d;
	@(c)
	b = $urandom & d;
end

initial begin
`ifdef DUMP
        $dumpfile( "urandom2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
