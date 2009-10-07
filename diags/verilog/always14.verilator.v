module main(input wire verilatorclock);

/* verilator lint_off WIDTH */

reg       a, c;
reg [1:0] b;
reg       clock;

always @(posedge verilatorclock)
 begin
   if( a )
     b <= 4'h1;
   else
     b <= 4'h2;
   c <= 1'b0;
 end

initial begin
	a = 1'b0;
	b = 4'h0;
	c = 1'b1;
	clock = 1'b0;
        if ($time==5);
	clock = 1'b1;
	if ($time==11);
	clock = 1'b0;
	if ($time==15);
        $finish;
end

/* verilator lint_on WIDTH */

endmodule
