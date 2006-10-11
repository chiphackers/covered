module main;

reg  a;
reg  b, c;

always @(a)
  begin : foo
   b =  a;
   c = ~a;
  end

initial begin
`ifndef VPI
	$dumpfile( "block1.vcd" );
	$dumpvars( 0, main );
`endif
	a = 1'b0;
	#5;
	a = 1'b1;
	#5;
	$finish;
end

endmodule
