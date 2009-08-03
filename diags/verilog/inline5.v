/*
 Name:        inline5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        08/03/2009
 Purpose:     Verifies bug fix for bug 2831553.
*/

module main;

reg a, b, c, d;

always @* begin : foobar
  a = b;
  case( b )
    1'b0:
      if( c )
        a = 1'b1;
    1'b1:
      begin
        case( 1'b1 )
          d: a = 1'b0;
        endcase
      end
  endcase
end

initial begin
`ifdef DUMP
        $dumpfile( "inline5.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b1;
	c = 1'b0;
	d = 1'b1;
        #10;
        $finish;
end

endmodule
