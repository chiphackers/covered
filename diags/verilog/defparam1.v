module main;

reg  in;
wire out;

initial begin
`ifdef DUMP
        $dumpfile( "defparam1.vcd" );
        $dumpvars( 0, main );
`endif
	in = 1'b0;
	#10;
	in = 1'b1;
	#10;
	in = 1'b0;
	#100;
	$finish;
end

defparam foo.a = 1'b1;

foobar foo(
  .y( in  ),
  .x( out )
);

endmodule

module foobar( y, x );

input  y;
output x;

parameter a = 1'b0;

assign x = y & a;

endmodule
