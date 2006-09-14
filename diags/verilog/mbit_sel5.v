module main;

reg  [15:0] a;
reg  [3:0]  b;
reg  [3:0]  c;

always @*
  b = a[c+:4];

initial begin
        $dumpfile( "mbit_sel5.vcd" );
        $dumpvars( 0, main );
	a = 16'h8421;
	c = 0;
	#10;
	c = 4;
	#10;
	c = 8;
	#10;
	c = 12;
        #10;
        $finish;
end

endmodule
