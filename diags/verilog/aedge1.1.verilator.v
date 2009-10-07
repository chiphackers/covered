/*
 Name:     aedge1.1.verilator.v
*/

module main(input wire verilatorclock);

reg [3:0]  a;
reg [31:0] b;

always @(a) begin
  	if( b === 32'bx )
    	b = 1;
  	if( $time > 0 )
    	b = b << 1;
end

always @(posedge verilatorclock) begin
	if ($time==1)
	a = 4'h0;
	if ($time==5)
	a = 4'h1;
	if ($time==11)
	a = 4'h1;
	if ($time==15)
	a = 4'h2;
	if ($time==21)
	a = 4'h2;
	if ($time==25)
	a = 4'h4;
	if ($time==31)
	a = 4'h8;
	if ($time==51)
	$finish;
end

endmodule
