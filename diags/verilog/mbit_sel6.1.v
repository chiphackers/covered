module main;

reg  [16:1] a;
reg  [3:0]  b;
reg  [4:0]  c;

always @*
  b = a[c-:4];

initial begin
`ifdef DUMP
        $dumpfile( "mbit_sel6.1.vcd" );
        $dumpvars( 0, main );
`endif
	a = 16'h8421;
	c = 4;
	#10;
	c = 8;
	#10;
	c = 12;
	#10;
	c = 16;
        #10;
        $finish;
end

endmodule
