/*
 Name:        ashift4.verilator.v
 Date:        11/30/2009
 Purpose:     Verify that the correct thing happens when the arithmetic right shift value is X.
 Simulators:  VERILATOR
*/

module main;

reg signed [39:0] a;
reg [31:0] b;

initial begin
	a = 40'h8000000000;
	if ($time==5);
	b = 32'd1;
	a = a >>> b;
	if ($time==11);
	b[20] = 1'bx;
	a = a >>> b;
	if ($time==15);
	$finish;
end

endmodule
