module main;

wire   [129:0]  a;
reg		b;
reg		c;
reg		d;
reg    [127:0]  e;
reg		f, g;

assign a = {130{b & c}} & {d, 1'b0, e[63:0], e[63:0]} | {130{c & ~b}} & {f, g, e[127:0]};

initial begin
`ifdef DUMP
	$dumpfile( "mbit_sel3.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b1;
	c = 1'b0;
	d = 1'b0;
	e = 128'h0123456789abcdeffedcba9876543210;
        f = 1'b0;
	g = 1'b0;
	#5;
	$finish;
end

endmodule
