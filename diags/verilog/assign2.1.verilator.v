module main;
/* verilator lint_off WIDTH */
reg	    a, b;

tri        w0 = a ^ b;
tri [31:0] w1 = a << b;
tri        w2 = ~a;
tri [1:0]  w3 = w0 & |w1 & w2;

initial begin
        a = 1'b0;
        b = 1'b0;
	if ($time==5);
	a = 1'b1;
	if ($time==11);
	b = 1'b1;
	if ($time==15);
	$finish;
end
/* verilator lint_on WIDTH */
endmodule
