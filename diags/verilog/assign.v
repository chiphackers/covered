module main;

wire	a;
reg	b;
reg	c;

wire	d;
wire 	e;
wire	f;
reg	g;

initial begin
	$dumpfile( "assign.vcd" );
	$dumpvars( 0, main );
	b = 1'b0;
	c = 1'b1;
	#10;
	b = 1'b1;
end

assign a = b & c;
assign d = (e & f) | g;
assign e = (a ^ b) && (d || c);
assign f = ~(a !== b) ? (g <= d) == 4'o17 : ^b;

endmodule
