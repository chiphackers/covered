module main;

reg  a, b;
wire c, z;

adder1 add (
  .a( a ),
  .b( b ),
  .c( c ),
  .z( z )
);

initial begin
`ifdef DUMP
	$dumpfile( "top.vcd" );
	$dumpvars( 0, main );
`endif
	a = 1'b0;
	b = 1'b0;
	#10;
	$finish;
end

endmodule
