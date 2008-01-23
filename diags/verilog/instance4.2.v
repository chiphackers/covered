module main;

reg    a, b;

foobar foo( a, b );

initial begin
`ifdef DUMP
	$dumpfile( "instance4.vcd" );
	$dumpvars( 0, main );
`endif
	a = 1'b0;
	b = 1'b0;
	#5;
	a = 1'b1;
	#5;
	a = 1'b0;
	b = 1'b1;
	#5;
	$finish;
end

endmodule


module foobar( b, c );

input  b, c;
wire   a;

assign a = b | c;

fooman foo( b, c );

endmodule


module fooman( b, c );

input   b;
input   c;

wire   a;
assign a = b & c;

endmodule
