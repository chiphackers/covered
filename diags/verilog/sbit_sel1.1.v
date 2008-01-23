module main;

wire [15:0] a;
wire        b;
wire [31:0] c;

reg  [31:0] vec;

assign a = vec[15:0];
assign b = vec[16];
assign c = vec;

initial begin
`ifdef DUMP
	$dumpfile( "sbit_sel1.1.vcd" );
	$dumpvars( 0, main );
`endif
	vec = 32'h01234567;
	#10;
	$finish;
end

endmodule
