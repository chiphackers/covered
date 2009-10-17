/*
 Name:        generate22.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/17/2009
 Purpose:     Instantiate two instances of the same module within a generate
              call.  Verifies fix for bug 2880705.
*/

module main;

reg  [3:0] a;
wire [3:0] b, c;
genvar     i;

generate
  for( i=0; i<4; i=i+1 ) begin : U
    foo f1( a[i], b[i] );
    foo f2( b[i], c[i] );
  end
endgenerate

initial begin
`ifdef DUMP
        $dumpfile( "generate22.vcd" );
        $dumpvars( 0, main );
`endif
	a = 4'b1010;
	#5;
	a = 4'b1001;
        #10;
        $finish;
end

endmodule

module foo (
  input  wire x,
  output wire y
);

assign y = ~x;

endmodule
