module main;

reg  x;
reg  a, b, c, d, e, f;

always
  begin
   #1;
   a  = x;
   b <= !x;
  end

always
  begin
   #1;
   c = x;
   d = ~x;
  end

always
  begin
   #1;
   e <= x;
   f <= ~x;
  end

initial begin
`ifdef DUMP
	$dumpfile( "race2.4.vcd" );
	$dumpvars( 0, main );
`endif
	x = 1'b0;
	#10;
	$finish;
end

endmodule
