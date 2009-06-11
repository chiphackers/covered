/*
 Name:        mem6.3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/11/2009
 Purpose:     Verifies that two memories within the same concatenation on the LHS
              side of a non-blocking assignment works properly.
*/

module main;

reg [3:0] a[0:2];

initial begin
	a[0] <= 4'h0;
	a[1] <= 4'h0;
	a[2] <= 4'h0;
	#5;
	{a[0],a[1],a[2]} <= 12'h123;
end

initial begin
`ifdef DUMP
        $dumpfile( "mem6.3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
