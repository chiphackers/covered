module main;

reg    a, b;

always @(a)
  begin
   b <= 1'b0;
   @(a);
   b <= 1'b1;
  end

initial begin
	$dumpfile( "always11.2.vcd" );
	$dumpvars( 0, main );
	a = 1'b0;
	#40;
	$finish;
end

endmodule
