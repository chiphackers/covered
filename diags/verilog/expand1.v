module main;

wire [4:0] a;
reg        b;

assign a = {5{b}};

initial begin
	$dumpfile( "expand1.vcd" );
	$dumpvars( 0, main );
	#5;
	b = 1'b0;
	#5;
/*
	b = 1'b1;
	#5;
	b = 1'b0;
	#5;
*/
	b = 1'bz;
	#5;
	$finish;
end

endmodule
