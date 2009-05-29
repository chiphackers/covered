/*
 Name:        mem6.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/26/2009
 Purpose:     Verifies that non-blocking assignments of memory elements is properly
              handled by Covered's simulation core.
*/

module main;

reg [3:0] a[0:1];
reg       b;

initial begin
	a[0] <= 4'h0;
	#5;
	a[0] <= 4'hc;
	#5;
	a[0] <= 4'h3;
end

initial begin
`ifdef DUMP
        $dumpfile( "mem6.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
	b = 1'b0;
	#5;
        $finish;
end

endmodule
