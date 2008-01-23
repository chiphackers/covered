module main;

reg  [16:1] a;
reg  [3:0]  b;
reg  [3:0]  c;

always @*
  b = a[c+:4];

initial begin
`ifdef DUMP
        $dumpfile( "mbit_sel5.1.vcd" );
        $dumpvars( 0, main );
`endif
	a = 16'h8421;
	c = 1;
	#10;
	c = 5;
	#10;
	c = 9;
	#10;
	c = 13;
        #10;
        $finish;
end

endmodule
