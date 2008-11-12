/*
 Name:        merge8.2a.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        11/02/2008
 Purpose:     Verifies that two CDD files generated from the same DUT but non-overlapping
              modules can be merged correctly.
*/

module main;

reg  [3:0] a, b;
wire [3:0] z;
wire       c;

adder4 adder( a, b, c, z );

initial begin
`ifdef DUMP
        $dumpfile( "merge8.2a.vcd" );
        $dumpvars( 0, main );
`endif
	a = 4'h0;
	b = 4'h0;
	#5;
	a = 4'hd;
	b = 4'hc;
        #10;
        $finish;
end

endmodule
