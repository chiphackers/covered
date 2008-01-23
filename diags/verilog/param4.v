module main;

parameter vec = 32'h01234567;

wire [15:0] a;
wire        b;
wire [31:0] c;

assign a = vec[15:0];
assign b = vec[16];
assign c = vec;

initial begin
`ifdef DUMP
	$dumpfile( "param4.vcd" );
	$dumpvars( 0, main );
`endif
	#10;
	$finish;
end

endmodule
