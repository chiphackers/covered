module main;

reg    a, b;

always @(negedge a)
  begin
   b <= 1'b0;
   @(negedge a);
   b <= 1'b1;
  end

initial begin
	$dumpfile( "always11.1.vcd" );
	$dumpvars( 0, main );
	a = 1'b1;
	#20;
	a = 1'b0;
	#40;
	$finish;
end

endmodule
