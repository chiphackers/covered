/*
 Name:        random4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        01/19/2009
 Purpose:     Verifies that random value concatenation works properly.
*/

module main;

reg [63:0] a;
reg        b;
reg        c;

initial begin
	a = 64'h0;
	b = 1'b0;
	c = 1'b0;
	#5;
	a = {$random, $random};
	if( a[31:0] != 32'h0 )
          b = 1'b1;
	if( a[63:32] != 32'h0 )
	  c = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "random4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
