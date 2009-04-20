/*
 Name:        generate16.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/24/2009
 Purpose:     Verify that generated logic with parameters work properly.
*/

module main;

parameter FOO = 0;
parameter BAR = 33;

generate
  if( FOO == 0 ) begin : foobar
    wire [BAR-1:0] a = {BAR{1'b0}};
  end
endgenerate

initial begin
`ifdef DUMP
        $dumpfile( "generate16.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
