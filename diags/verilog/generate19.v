/*
 Name:        generate19.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/15/2009
 Purpose:     Verify that generated module instances with their
              own subscopes are generated correctly.
*/

module main;

genvar     i;
reg  [1:0] a;
wire [1:0] b;
reg        clock;

generate
  for( i=0; i<2; i=i+1 ) begin : U
    foo f( clock, a[i], b[i] );
  end
endgenerate

initial begin
`ifdef DUMP
        $dumpfile( "generate19.vcd" );
        $dumpvars( 0, main );
`endif
	a = 2'b01;
	#5;
	a = 2'b10;
        #10;
        $finish;
end

initial begin
	clock = 1'b0;
	forever #(2) clock = ~clock;
end

endmodule

//-------------------------------------------------

module foo(
  input  wire clock,
  input  wire a,
  output reg  b
);

always @(posedge clock) begin
  b <= ~a;
end

endmodule
