module main;

foo #(3,,4) a();

initial begin
`ifndef VPI
	$dumpfile( "param12.vcd" );
	$dumpvars( 0, main );
`endif
	#10;
	$finish;
end

endmodule

module foo;

parameter A=10,
          B=20,
          C=30;

wire [(A-1):0] a;
wire [(B-1):0] b;
wire [(C-1):0] c;

initial begin
	$display( "A: %h, B: %h, C: %h", A, B, C );
end

endmodule
