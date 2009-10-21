/*
 Name:        generate24.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/20/2009
 Purpose:     Verifies parameter overriding within a generate block works when the
              name matches in both the parent and child module.
*/

module main;

parameter FOO = 4;

genvar    i;
reg [3:0] a;

generate
  for( i=0; i<4; i=i+1 ) begin : U
    foobar #(.FOO(FOO)) f( a[i] );
  end
endgenerate

initial begin
`ifdef DUMP
        $dumpfile( "generate24.vcd" );
        $dumpvars( 0, main );
`endif
	a = 4'b1010;
	#5;
	a = 4'b1001;
        #10;
        $finish;
end

endmodule

//-------------------------------------------------

module foobar (
  input x
);

parameter FOO = 1;

wire [FOO-1:0] z = {FOO{~x}};

endmodule
