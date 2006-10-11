module main;

event a, b;
reg [31:0] i;

initial begin
	fork
	  for( i=0; i<5; i=i+1 )
            @(a);
          @(b);
        join
end

initial begin
	#10;
	->a;
	#10;
	->a;
	->b;
	#10;
	->a;
	#10;
	->a;
	->a;
	#10;
end

initial begin
`ifndef VPI
        $dumpfile( "for2.vcd" );
        $dumpvars( 0, main );
`endif
	#100;
	$finish;
end

endmodule
