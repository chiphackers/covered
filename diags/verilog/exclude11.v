/*
 Name:        exclude11.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/17/2008
 Purpose:     See script for details.
*/

module main;

reg a, b, c, d;

initial begin
	@(posedge a);
	b = 1'b0;
	c = 1'b1;
	d = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "exclude11.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
