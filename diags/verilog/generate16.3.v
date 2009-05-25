/*
 Name:        generate16.3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/22/2009
 Purpose:     Verify that parameter overrides that are generated from genvar expressions
              for generated instances works correctly.
*/

module main;

reg [2:0] a;

generate
  genvar i;
  for( i=0; i<2; i=i+1 ) begin : u
    foo #(i+1) f( a );
  end
endgenerate

initial begin
`ifdef DUMP
        $dumpfile( "generate16.3.vcd" );
        $dumpvars( 0, main );
`endif
	a = 3'h0;
	#5;
	a = 3'h1;
	#5;
	a = 3'h2;
        #10;
        $finish;
end

endmodule

module foo (
  input [2:0] a
);

parameter BIT=0;

wire b = ~a[BIT];

endmodule
