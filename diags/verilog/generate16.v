/*
 Name:        generate16.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/23/2009
 Purpose:     Verifies that modules containing parameters whose instantiations occur within
              conditional generate statements work properly.
*/

module main;

parameter FOO = 0;

generate
  if( FOO == 0 ) begin
    bar b( 1'b0 );
  end else begin
    foo f( 1'b1 );
  end
endgenerate

initial begin
`ifdef DUMP
        $dumpfile( "generate16.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule

module bar(
  input x
);

parameter BUBBA = 32;

wire [BUBBA-1:0] a = {BUBBA{x}};

endmodule


module foo(
  input y
);

parameter BUBBA = 64;

wire [BUBBA-1:0] a = {BUBBA{~y}};

endmodule
