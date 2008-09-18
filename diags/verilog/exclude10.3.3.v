/*
 Name:        exclude10.3.3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/17/2008
 Purpose:     See script for details.
*/

module main;

reg a, b;

initial begin
	@(posedge a);
	b = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "exclude10.3.3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
