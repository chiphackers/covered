module main;

reg a, b, c, clk;

always @(b) a = b;

always @(posedge clk)
  begin
   a <= 1'b0;
   b <= 1'b1;
   c <= $random;
  end

initial begin
        $dumpfile( "race3.2.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

initial begin
	clk = 1'b0;
	forever #(1) clk = ~clk;
end

endmodule
