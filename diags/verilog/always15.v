/*
 Name:        always15.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/04/2009
 Purpose:     Verifies that always blocks with bit selects trigger properly.
*/

module main;

reg [3:0] a, b;

always @(a[0]) begin
  b = a;
end

initial begin
`ifdef DUMP
        $dumpfile( "always15.vcd" );
        $dumpvars( 0, main );
`endif
	a = 4'h0;
	#5;
	a = 4'h1;
	#5;
	a = 4'h2;
	#5;
	a = 4'h4;
	#5;
	a = 4'h8;
        #10;
        $finish;
end

endmodule
