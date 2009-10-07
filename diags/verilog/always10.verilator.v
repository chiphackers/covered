module main (input wire verilatorclock);
/* verilator lint_off COMBDLY */
reg  a, b, c;

always @(posedge verilatorclock)
  if( a )
    b <= c;

always @(verilatorclock) begin
if ($time==0) begin
	a = 1'b0;
	c = 1'b0;	
end

else if ($time>=20) a <= 1'b1; 
else if ($time>=21) a <= 1'b0; 
else if ($time==41) $finish;

end
/* verilator lint_on COMBDLY */
endmodule
