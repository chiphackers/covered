module main;

reg    a, b;

always @(posedge a)
  begin
   b <= 1'b0;
   @(posedge a);
   b <= 1'b1;
  end

initial begin
`ifdef DUMP
	$dumpfile( "always11.vcd" );
	$dumpvars( 0, main );
`endif
	a = 1'b0;
	#20;
	a = 1'b1;
	#40;
	$finish;
end

endmodule
