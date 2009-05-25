/*
 Name:        generate16.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/22/2009
 Purpose:     Verify that parameter overrides for generated instances works correctly.
*/

module main;

reg [2:0] a;

generate
  genvar i;
  for( i=0; i<2; i=i+1 ) begin : u
    foo #(0) f( a );
  end
endgenerate

initial begin
`ifdef DUMP
        $dumpfile( "generate16.2.vcd" );
        $dumpvars( 0, main );
`endif
	a = 3'h0;
	#5;
	a = 3'h1;
        #10;
        $finish;
end

endmodule

module foo (
  input [2:0] a
);

parameter BIT=2;

wire b = ~a[BIT];

endmodule
