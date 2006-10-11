module main;

wire    qo, ro, so, to, uo, vo, wo, xo;

wire    q1, r1, s1, t1, u1, v1;

reg     s2;
reg     t2;
reg     u2;
reg     v2;

reg     w1, w2, w3;
reg     x1, x2, x3;


tran( qo, q1 );
rtran( ro, r1 );
tranif0( so, s1, s2 );
tranif1( to, t1, t2 );
rtranif0( uo, u1, u2 );
rtranif1( vo, v1, v2 );

cmos( wo, w1, w2, w3 );
rcmos( xo, x1, x2, x3 );

initial begin
`ifndef VPI
	$dumpfile( "gate1.1.vcd" );
	$dumpvars( 0, main );
`endif
	s2 = 1'b0;
	t2 = 1'b0;
	u2 = 1'b0;
	v2 = 1'b0;
	w1 = 1'b0;
	w2 = 1'b0;
	w3 = 1'b0;
	x1 = 1'b0;
	x2 = 1'b0;
	x3 = 1'b0;
	#5;
	s2 = 1'b1;
	t2 = 1'b1;
	u2 = 1'b1;
	v2 = 1'b1;
	w2 = 1'b1;
	x2 = 1'b1;
	#5;
	$finish;
end

endmodule
