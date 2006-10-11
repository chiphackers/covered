module main;

event  a;
event  b, c;

reg    d, e, f;

always @(a)
  begin
   ->b;
   #5;
   ->c;
  end

always @(b) e = ~d;
always @(c) f = ~d;

initial begin
	d = 1'b0;
	#5;
	->a;
	#2;
	d = 1'b1;
end

initial begin
`ifndef VPI
	$dumpfile( "event1.1.vcd" );
	$dumpvars( 0, main );
`endif
	d = 1'b0;
	#25;
	$finish;
end

endmodule
