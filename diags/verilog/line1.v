module main;


`line 1 "inc_body.v" 1
reg	b, c;
wire	a;

assign a = b & c;

initial begin
	b = 1'b0;
	c = 1'b0;
	#5;
	c = 1'b1;
	#5;
	c = 1'b0;
	#5;
	b = 1'b1;
	#5;
	c = 1'b1;
	#5;
	$finish;
end

`line 4 "include2.v" 2

initial begin
	$dumpfile( "line1.vcd" );
	$dumpvars( 0, main );
end

endmodule
