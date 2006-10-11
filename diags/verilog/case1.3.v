module main;

reg	clock;
reg	a, b;

always @(posedge clock)
  case( a )
    1'b0 :  b <= 1'b1;
    1'b1 :  b <= 1'b0;
  endcase

initial begin
`ifndef VPI
	$dumpfile( "case1.3.vcd" );
	$dumpvars( 0, main );
`endif
	clock = 1'b0;
	forever #(5) clock = ~clock;
end

initial begin
	#20;
	$finish;
end

endmodule
