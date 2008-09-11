/*
 Name:        negate2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/10/2008
 Purpose:     Verifies that decimal negation is output properly in the report files.
*/

module main;

reg [31:0] a, b;

initial begin
	a = 32'h01234567;
	b = a & -1;
end

initial begin
`ifdef DUMP
        $dumpfile( "negate2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
