/*
 Name:        exclude10.3.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/15/2008
 Purpose:     See script for details.
*/

module main;

wire a_and, a_or;
reg  b, c, d;

assign a_and = b & c & d;
assign a_or  = b | c | d;

initial begin
`ifdef DUMP
        $dumpfile( "exclude10.3.1.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
	c = 1'b1;
	d = 1'b0;
        #10;
        $finish;
end

endmodule
