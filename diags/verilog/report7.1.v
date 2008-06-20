/*
 Name:        report7.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/20/2008
 Purpose:     See script for details.
*/

module main;

reg [15:0] b;

wire a = b[ 0] | b[ 1] | b[ 2] | b[ 3] |
         b[ 4] | b[ 5] | b[ 6] | b[ 7] |
         b[ 8] | b[ 9] | b[10] | b[11] |
         b[12] | b[13] | b[14] | b[15];

initial begin
`ifdef DUMP
        $dumpfile( "report7.1.vcd" );
        $dumpvars( 0, main );
`endif
	b = 16'h0123;
        #10;
        $finish;
end

endmodule
