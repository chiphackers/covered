module main;

parameter [15:0] a = 16'h8421;

reg  [3:0]  b;
reg  [3:0]  c;

always @*
  b = a[c-:4];

initial begin
`ifdef DUMP
        $dumpfile( "mbit_sel6.2.vcd" );
        $dumpvars( 0, main );
`endif
	c = 3;
	#10;
	c = 7;
	#10;
	c = 11;
	#10;
	c = 15;
        #10;
        $finish;
end

endmodule
