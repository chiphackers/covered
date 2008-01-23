module main;

reg clk;
reg resetn;
reg q, d;

always_ff @(posedge clk or negedge resetn)
  if( !resetn )
    q <= 1'b0;
  else
    q <= d;

initial begin
`ifdef DUMP
        $dumpfile( "always_ff1.vcd" );
        $dumpvars( 0, main );
`endif
	resetn = 1'b0;
	d = 1'b1;
        #10;
	resetn = 1'b1;
	#10;
        $finish;
end

initial begin
	clk = 1'b0;
	forever #(1) clk = ~clk;
end

endmodule
