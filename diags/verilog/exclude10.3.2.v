/*
 Name:        exclude10.3.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/17/2008
 Purpose:     See script for details.
*/

module main;

wire a;
reg  b, c;

assign a = (b == c);

initial begin
`ifdef DUMP
        $dumpfile( "exclude10.3.2.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
	c = 1'b1;
        #10;
        $finish;
end

endmodule
