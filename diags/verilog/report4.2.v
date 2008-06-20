/*
 Name:        report4.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/19/2008
 Purpose:     See script for details.
*/

module main;

reg b, c, d;

wire a = ((b | c) & d) | ((b & c) & d) | ((b | ~c) & ~d);

initial begin
`ifdef DUMP
        $dumpfile( "report4.2.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b1;
	c = 1'b0;
	d = 1'b0;
        #10;
        $finish;
end

endmodule
