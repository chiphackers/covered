module main;

reg  [17:0]   a;

integer i;

initial begin
`ifndef VPI
	$dumpfile( "toggle.vcd" );
	$dumpvars( 0, main );
`endif
        a = 18'h00000;
	for( i=0; i<9; i=i+1 )
          begin
           #10;
	   a = (1 << (i * 2));
          end
	#10;
	$finish;
end

endmodule
