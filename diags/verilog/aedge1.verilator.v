/*
 Name:     aedge1.verilaotr.v
*/

module main;

reg a;
reg [31:0] b;

initial begin
	if ($time == 1)
	b = 1;
end

always @(a) b = b << 1; //Rewrite from initial block

initial begin
	if ($time == 0)
	a = 1'b0;
	if ($time == 5)
	a = 1'b1;
	if ($time == 10)
	a = 1'b1;
	if ($time == 15)
	a = 1'b0;
end

initial begin
        if ($time == 25)
        $finish;
end

endmodule
