/*
 Name:        generate21.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/23/2009
 Purpose:     Verifies that two instances containing generate blocks controlled by parameters that are
              different for each instance works correctly.
*/

module main;

reg a, b;

foo #(0) x( a );
foo #(1) y( b );

initial begin
`ifdef DUMP
        $dumpfile( "generate21.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1'b0;
	b = 1'b1;
	#5;
	a = 1'b1;
	b = 1'b0;
        #10;
        $finish;
end

endmodule

//-------------------------------------------

module foo(
  input wire a
);

parameter select = 0;

generate
  if( select == 0 ) begin : U
    wire b = a;
  end else begin : V
    wire c = ~a;
  end
endgenerate

endmodule

