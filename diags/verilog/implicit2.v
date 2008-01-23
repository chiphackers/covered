module main;

integer   i;
time      t;
reg [1:0] mem[1:0];

wire [1:0] a, b, c;

assign a = i;
assign b = t;
assign c = mem[0];

initial begin
`ifdef DUMP
	$dumpfile( "implicit2.vcd" );
	$dumpvars( 0, main );
`endif
	i      = 1;
	t      = 2;
	mem[0] = 2'h0;
	#5;
	i      = 10;
	t      = 20;
	mem[1] = 2'h1;
	#5;
	$finish;
end

endmodule
