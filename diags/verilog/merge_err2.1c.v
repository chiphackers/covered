/*
 Name:        merge_err2.1c.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        08/05/2008
 Purpose:     See script for details.
*/

module main_c;

reg  a, b;
wire c, z;

adder1 adder (
  .a(a),
  .b(b),
  .c(c),
  .z(z)
);

initial begin
`ifdef DUMP
        $dumpfile( "merge_err2.1c.vcd" );
        $dumpvars( 0, main_c );
`endif
	a = 1'b1;
	b = 1'b0;
        #10;
	a = 1'b0;
        $finish;
end

endmodule
