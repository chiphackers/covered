/*
 Name:        generate19.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/15/2009
 Purpose:     Verify that a module instantiated within both a generate block and
              outside of a generate block are handled properly.
*/

module main;

reg  [2:0] a;
wire [2:0] b;
reg        clock;
genvar     i;

generate
  for( i=0; i<2; i=i+1 ) begin : U
    foo f( clock, a[i], b[i] );
  end
endgenerate

foo f( clock, a[2], b[2] );

initial begin
`ifdef DUMP
        $dumpfile( "generate19.1.vcd" );
        $dumpvars( 0, main );
`endif
	a = 3'b101;
	#5;
	a = 3'b010;
        #5;
        $finish;
end

initial begin
	clock = 1'b0;
	forever #(2) clock = ~clock;
end

endmodule

//--------------------------------------

module foo(
  input  wire clock,
  input  wire a,
  output reg  b
);

always @(posedge clock)
  b <= ~a;

endmodule
