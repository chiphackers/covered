/*
 Name:        exclude10.3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/10/2008
 Purpose:     See script for details.
*/

module main;

wire a, b;
reg  c, d;

assign a = c & d;
assign b = c | d;

initial begin
`ifdef DUMP
        $dumpfile( "exclude10.3.vcd" );
        $dumpvars( 0, main );
`endif
	c = 1'b0;
	d = 1'b0;
	#5;
	d = 1'b1;
        #10;
        $finish;
end

endmodule
