module main;

reg       a;
reg [1:0] b;

initial begin
        $dumpfile( "simple.vcd" );
        $dumpvars( 0, main );
        a = 1'b0;
        b = 2'h0;
	#5;
	if( a )
          b = 2'b10;
        else
          b = 2'b01;
        #5;
end

endmodule
