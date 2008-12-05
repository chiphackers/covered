module main;

reg       a, b, c, d, e, f;
reg [3:0] z;

wire x = b ^ c;
wire y;

assign y = &z;

initial begin #1 a = b & c; end

initial begin : foo
	#1;
	d = b | c;
end

initial #1 f = b && c;

always @(posedge b)
  if( c )
    e <= a & d;
  else
    e <= a | d;

initial begin
`ifdef DUMP
	$dumpfile( "test.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b0;
	c = 1'b1;
	#5;
	b = 1'b1;
	#10;
	$finish;
  end
endmodule
