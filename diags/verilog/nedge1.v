/*
 Name:        nedge1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/11/2008
 Purpose:     Verifies that a missed embedded negedge operator is output to the report file correctly.
*/

module main;

reg a, b, c, d;

always @(negedge a or negedge b)
  c <= ~d;

initial begin
`ifdef DUMP
        $dumpfile( "nedge1.vcd" );
        $dumpvars( 0, main );
`endif
	d = 1'b0;
        #10;
        $finish;
end

endmodule
