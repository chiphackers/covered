module main;

tri1       a;
tri1 [1:0] b;
wire [1:0] c;
reg        d, e;

assign a = d & e;
assign b = {d, e};
assign c = {2{a}} | b;

initial begin
	$dumpfile( "tri1.vcd" );
	$dumpvars( 0, main );
	d = 1'b0;
	e = 1'b1;
	#5;
	d = 1'b1;
	e = 1'b0;
	#5;
	$finish;
end

endmodule 
