module main;

wire    ao, bo, co, do, eo, fo, go, ho, io, jo, ko, lo, mo, no, oo, po;

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


and( ao, a1, a2 );
nand( bo, b1, b2 );
or( co, c1, c2 );
nor( do, d1, d2 );
xor( eo, e1, e2 );
xnor( fo, f1, f2 );

buf( go, g1 );
not( ho, h1 );

bufif0( io, i1, i2 );
bufif1( jo, j1, j2 );
notif0( ko, k1, k2 );
notif1( lo, l1, l2 );

nmos( mo, m1, m2 );
pmos( no, n1, n2 );
rnmos( oo, o1, o2 );
rpmos( po, p1, p2 );


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
