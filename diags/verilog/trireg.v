module main;

trireg                a;
trireg (small)        a1;
trireg (medium)       a2;
trireg (large)        a3;
trireg          [1:0] b;
trireg (small)  [1:0] b1;
trireg (medium) [1:0] b2;
trireg (large)  [1:0] b3;
wire            [1:0] c, c1, c2, c3;
reg                   d, e;

assign a = d & e;
assign b = {d, e};
assign c = {2{a}} | b;

assign a1 = d & e;
assign b1 = {d, e};
assign c1 = {2{a1}} | b1;

assign a2 = d & e;
assign b2 = {d, e};
assign c2 = {2{a2}} | b2;

assign a3 = d & e;
assign b3 = {d, e};
assign c3 = {2{a3}} | b3;

initial begin
`ifndef VPI
	$dumpfile( "trireg.vcd" );
	$dumpvars( 0, main );
`endif
	d = 1'b0;
	e = 1'b1;
	#5;
	d = 1'b1;
	e = 1'b0;
	#5;
	$finish;
end

endmodule 
