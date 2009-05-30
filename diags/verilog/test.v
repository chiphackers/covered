/*
 Name:        test.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/29/2009
 Purpose:     
*/

module main;

reg a, b, c;
reg clock;

always @(posedge clock) if( b ) a <= b;
always @(posedge clock) if( b ) c <= b;

initial begin
`ifdef DUMP
        $dumpfile( "test.vcd" );
        $dumpvars( 0, main );
`endif
	b     = 1'b0;
	clock = 1'b0;
	#10;
	clock = 1'b1;
        #10;
        $finish;
end

endmodule
