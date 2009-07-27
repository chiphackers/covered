/*
 Name:        merge12a.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        07/27/2009
 Purpose:     See script for details.
*/

module main;

reg [3:0] a;

merge12_dut dut( a );

initial begin
`ifdef DUMP
	$dumpfile( "merge12a.vcd" );
	$dumpvars( 0, main );
`endif
	a = 4'h1;
	#5;
	$finish;
end

endmodule
