module main;

wire [1:0]   a;
wire         b;
wire         c;
reg  [3:0]   d;

assign {a, b, c} = d;

initial begin
	$dumpfile( "concat3.vcd" );
	$dumpvars( 0, main );
	d = 4'h0;
	#5;
	d = 1'h1;
	#5;
	d = 1'h2;
	#5;
	d = 4'h4;
	#5;
	d = 4'h8;
	$finish;
end

endmodule
