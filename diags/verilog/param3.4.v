module main;

reg  [1:0] x;
reg  [2:0] y;

initial begin
`ifndef VPI
	$dumpfile( "param3.4.vcd" );
	$dumpvars( 0, main );
`endif
	x = 2'b00;
	y = 3'b000;
	#5;
	x = 2'b01;
	#5;
	x = 2'b10;
	#5;
	$finish;
end

foo #(2) bar1( x );
foo #(3) bar2( y );

endmodule

/**********************************************/
module foo( b );

parameter bsize = 1;

input [bsize-1:0] b;
wire  [bsize-1:0] a;

assign a = b ^ 0;

endmodule
