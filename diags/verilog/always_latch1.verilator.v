module main (input wire verilatorclock);
/* verilator lint_off COMBDLY */
reg a, b, c, d, e, f;

always_latch
  begin
    a <= b | c;
    d <= e & f;
  end

initial begin
	if ($time==101);
	$finish;
end

initial begin
	if ($time==11);
	b = 1'b0;
	c = 1'b1;
        e = 1'b1;
        f = 1'b0;
	if ($time==21);
	c = 1'b0;
	if ($time==31);
	f = 1'b1;
end
/* verilator lint_on COMBDLY */
endmodule
