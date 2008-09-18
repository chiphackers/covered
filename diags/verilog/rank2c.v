/*
 Name:        rank2c.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/18/2008
 Purpose:     See script for details.
*/

module main;

reg  a, b;
wire c, z;

adder1 adder( 
  .a(a),
  .b(b),
  .c(c),
  .z(z)
);

initial begin
`ifdef DUMP
        $dumpfile( "rank2c.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1'b0;
	b = 1'b1;
        #10;
	a = 1'b1;
        $finish;
end

endmodule
