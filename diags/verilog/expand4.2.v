module main;

`define VALUE  3

wire [3:0] a;
reg        b;

assign a = {`VALUE+1{1'b0}};

initial begin
	$dumpfile( "expand4.2.vcd" );
	$dumpvars( 0, main );
	#5;
	b = 1'b0;
	#5;
	b = 1'bz;
	#5;
	$finish;
end

endmodule
