/*
 Name:        exclude10.5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/10/2008
 Purpose:     See script for details.
*/

module main;

reg       clock, reset_n;
reg [3:0] a;
  
assert_zero_one_hot #(.width(4)) foo (
  clock,     
  reset_n,
  a
);

initial begin
`ifdef DUMP
        $dumpfile( "exclude10.5.vcd" );
        $dumpvars( 0, main );
`endif
        reset_n = 1'b0;
        a       = 4'h0;
        #10;
        reset_n = 1'b1;
        #20;
        $finish;
end

initial begin
        clock = 1'b0;
        forever #1 clock = ~clock;
end

endmodule
