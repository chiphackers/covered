module main;

localparam a = 2;

generate
  if( a < 1 )
    foobar bar();
endgenerate

initial begin
        $dumpfile( "generate2.1.vcd" );
        $dumpvars( 0, main );
	#20;
	$finish;
end

endmodule

//---------------------------------

module foobar;

reg x;

endmodule
