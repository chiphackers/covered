/*
 Name:        mem6.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/27/2009
 Purpose:     Verify that memories used in single-bit selects on the LHS
              side of non-blocking assignments are not used.
*/

module main;

reg [3:0]  a[0:1];
reg [15:0] b;

initial begin
	a[0] <= 4'h0;
	#5;
	b[a[0]] <= 1'b0;
	#5;
	b[a[0]] <= 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "mem6.1.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
