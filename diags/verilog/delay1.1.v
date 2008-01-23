module main;

reg       b;
reg [1:0] a, c;

always @(posedge b) #(5:10:15) a <= c[1] & c[0];

initial begin
`ifdef DUMP
	$dumpfile( "delay1.vcd" );
	$dumpvars( 0, main );
`endif
	b = 1'b0;
	#5;
	b = 1'b1;
	#20;
	$finish;
end

initial begin
	c = 2'b00;
	#12;
        c = 2'b01;
        #5;
	c = 2'b10;
end

endmodule
