/*
 Name:        mem6.4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/11/2009
 Purpose:     Verifies that memory assignment within a concatentation works when other LSB elements
              are parameterized in their width.
*/

module main;

parameter foo = 1;

reg [3:0]     a[1:0];
reg [foo-1:0] b;

initial begin
	a[0] <= 4'h0;
	#5;
	{a[0],b} <= 5'h2;
end

initial begin
`ifdef DUMP
        $dumpfile( "mem6.4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
