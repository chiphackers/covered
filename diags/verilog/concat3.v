module main;

wire [1:0]   a;
wire         b;
wire         c;
reg  [3:0]   d;

assign {a, b, c} = d;

initial begin
`ifdef DUMP
	$dumpfile( "concat3.vcd" );
	$dumpvars( 0, main );
`endif
	d = 4'h0;
	#5;
	d = 4'h1;
	#5;
	d = 4'h2;
	#5;
	d = 4'h4;
	#5;
	d = 4'h8;
	#5;
	$finish;
end

endmodule
