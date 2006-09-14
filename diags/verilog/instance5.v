module main;

reg [2:0] a, b;

foobar foo( a );
foobar bar( b );

initial begin
	$dumpfile( "instance5.vcd" );
	$dumpvars( 0, main );
	a = 3'b000;
        b = 3'b111;
	#5;
	a = 3'b001;
	#5;
	a = 3'b100;
	#5;
	$finish;
end

endmodule


module foobar( b );

input [2:0]  b;

wire [1:0] a;

assign a = b[2:1];

endmodule
