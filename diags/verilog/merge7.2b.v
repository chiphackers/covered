/*
 Name:        merge7.2b.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/22/2008
 Purpose:     See ../regress/merge7.2.pl for details.
*/

module main;

wire a;
reg  b, c;

dut_and dut(
  .a(a),
  .b(b),
  .c(c)
);

initial begin
`ifdef DUMP
        $dumpfile( "merge7.2b.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
	c = 1'b1;
        #10;
        $finish;
end

endmodule
