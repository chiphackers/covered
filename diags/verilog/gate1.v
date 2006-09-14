module main;

wire    a_o, b_o, c_o, d_o, e_o, f_o, g_o, h_o, i_o, j_o, k_o, l_o, m_o, n_o, o_o, p_o;

reg     a1, a2;
reg     b1, b2;
reg     c1, c2;
reg     d1, d2;
reg     e1, e2;
reg     f1, f2;

reg     g1;
reg     h1;

reg     i1, i2;
reg     j1, j2;
reg     k1, k2;
reg     l1, l2;

reg     m1, m2;
reg     n1, n2;
reg     o1, o2;
reg     p1, p2;


and( a_0, a1, a2 );
nand( b_o, b1, b2 );
or( c_o, c1, c2 );
nor( d_o, d1, d2 );
xor( e_o, e1, e2 );
xnor( f_o, f1, f2 );

buf( g_o, g1 );
not( h_o, h1 );

bufif0( i_o, i1, i2 );
bufif1( j_o, j1, j2 );
notif0( k_o, k1, k2 );
notif1( l_o, l1, l2 );

nmos( m_o, m1, m2 );
pmos( n_o, n1, n2 );
rnmos( o_o, o1, o2 );
rpmos( p_o, p1, p2 );


initial begin
	$dumpfile( "gate1.vcd" );
	$dumpvars( 0, main );
	a1 = 1'b0;
	a2 = 1'b0;
	b1 = 1'b0;
	b2 = 1'b0;
	c1 = 1'b0;
	c2 = 1'b0;
	d1 = 1'b0;
	d2 = 1'b0;
	e1 = 1'b0;
	e2 = 1'b0;
	f1 = 1'b0;
	f2 = 1'b0;
	g1 = 1'b0;
	h1 = 1'b0;
	i1 = 1'b0;
	i2 = 1'b0;
	j1 = 1'b0;
	j2 = 1'b0;
	k1 = 1'b0;
	k2 = 1'b0;
	l1 = 1'b0;
	l2 = 1'b0;
	m1 = 1'b0;
	m2 = 1'b0;
	n1 = 1'b0;
	n2 = 1'b0;
	o1 = 1'b0;
	o2 = 1'b0;
	p1 = 1'b0;
	p2 = 1'b0;
	#5;
	a1 = 1'b1;
	b1 = 1'b1;
	c1 = 1'b1;
	d1 = 1'b1;
	e1 = 1'b1;
	f1 = 1'b1;
	g1 = 1'b1;
	h1 = 1'b1;
	i1 = 1'b1;
	j1 = 1'b1;
	k1 = 1'b1;
	l1 = 1'b1;
	m1 = 1'b1;
	n1 = 1'b1;
	o1 = 1'b1;
	p1 = 1'b1;
	#5;
	$finish;
end

endmodule
