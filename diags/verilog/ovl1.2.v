module main;

wire      clock;
reg       reset_n;
reg [3:0] a;

clock_gen clkgen (clock);

assert_zero_one_hot #(.width(4)) foo (
  clock,
  reset_n,
  a
);

initial begin
        $dumpfile( "ovl1.2.vcd" );
        $dumpvars( 0, main );
	reset_n = 1'b0;
	a       = 4'h1;
        #10;
	reset_n = 1'b1;
	#10;
	@(posedge clock) a <= 4'h0;
	#20;
        $finish;
end

endmodule


module clock_gen (
  output reg clock
);

initial begin
        clock = 1'b0;
        forever #1 clock = ~clock;
end

endmodule
