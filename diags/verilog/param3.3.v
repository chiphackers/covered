`define BITSIZE   2

module main;

parameter xsize = `BITSIZE;

reg  [xsize-1:0] x;

initial begin
`ifndef VPI
	$dumpfile( "param3.3.vcd" );
	$dumpvars( 0, main );
`endif
	x = `BITSIZE'b0;
	#5;
	x = `BITSIZE'b1;
	#5;
	x = `BITSIZE'b0;
	#5;
	$finish;
end

foo #(xsize) bar1( x );

endmodule

/**********************************************/
module foo( b );

parameter bsize = 1;

input [bsize-1:0] b;
wire  [bsize-1:0] a;

assign a = b ^ `BITSIZE'b0;

endmodule
