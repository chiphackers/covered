module main;

reg       a, c;
reg [1:0] b;
reg       clock;

always @(posedge clock)
 begin
   if( a )
     b <= 4'h1;
   else
     b <= 4'h2;
   c <= 1'b0;
 end

initial begin
        $dumpfile( "always14.vcd" );
        $dumpvars( 0, main );
	a = 1'b0;
	b = 4'h0;
	c = 1'b1;
	clock = 1'b0;
        #5;
	clock = 1'b1;
	#5;
	clock = 1'b0;
	#5;
        $finish;
end

endmodule
