module adder1( 
	a,
	b,
	c,
	z
);

input		a;
input		b;
output		c;
output	 	z;

wire      c, z;

assign z = a ^ b;
assign c = a & b;

endmodule
