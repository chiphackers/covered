module main;

parameter        incr = 4;
parameter [15:0] a    = 16'h8421;

reg  [3:0]  b;
reg  [3:0]  c;

always @*
  b = a[c+:incr];

initial begin
`ifndef VPI
        $dumpfile( "mbit_sel5.3.vcd" );
        $dumpvars( 0, main );
`endif
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
