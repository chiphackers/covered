module adder4( 
	a,
	b,
	c,
	z
);

input	[3:0]	a;
input	[3:0]	b;
output		c;
output	[3:0]   z;

wire    	c0, c1, c2;
wire    	c;
wire    [3:0]	z;

adder1 bit0( a[0], b[0], c0, z[0] );
adder1 bit1( a[1], b[1], c1, z[1] );
adder1 bit2( a[2], b[2], c2, z[2] );
adder1 bit3( a[3], b[3], c,  z[3] );

endmodule
