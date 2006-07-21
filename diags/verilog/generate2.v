module main;

localparam a = 0;

generate
  if( a < 1 )
    foobar bar();
endgenerate

initial begin
        $dumpfile( "generate2.vcd" );
        $dumpvars( 0, main );
	#20;
	$finish;
end

initial begin
	bar.x = 1'b0;
	#10;
	bar.x = 1'b1;
end

endmodule

//---------------------------------

module foobar;

reg x;

endmodule
