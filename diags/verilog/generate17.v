/*
 Name:        generate17.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/02/2009
 Purpose:     Verifies that nets created within generated named blocks are handled properly.
*/

module main;

localparam FOO = 0;

reg b, c;

generate
  if( FOO == 0 ) begin : bar
    wire a = b & c;
  end
endgenerate

initial begin
`ifdef DUMP
        $dumpfile( "generate17.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b1;
	c = 1'b1;
	#5;
	c = 1'b0;
        #10;
        $finish;
end

endmodule
