/*
 Name:        inline3.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        07/25/2009
 Purpose:     Verify that the -inline-metrics l option only allows line coverage
              for -inline runs.
*/

module main;

reg [1:0] a[0:1];
reg       b, c, d;
reg       clock;
reg       reset_n;

initial begin
	a[0] = 2'b0;
	@(posedge d);
	a[0] = {2{b | c}};
end

fsm fsm (
  .clock( clock ),
  .reset( ~reset_n ),
  .head ( c ),
  .tail ( b ),
  .valid( d )
);

assert_zero_one_hot #(.width(2)) foo (
  clock,
  reset_n,
  {b, c}
);

initial begin
	clock = 1'b0;
	forever #(2) clock = ~clock;
end

initial begin
`ifdef DUMP
        $dumpfile( "inline3.1.vcd" );
        $dumpvars( 0, main );
`endif
	reset_n = 1'b0;
	b       = 1'b0;
	c       = 1'b1;
	d       = 1'b0;
	#5;
	reset_n = 1'b1;
        #5;
	d       = 1'b1;
	#5;
        $finish;
end

endmodule
