module main;

wire [1:0] a, b;
reg  [3:0] c, d;

assign {a, b} = c & d;

initial begin
`ifndef VPI
        $dumpfile( "concat3.1.vcd" );
        $dumpvars( 0, main );
`endif
	c = 4'b0000;
	d = 4'b0000;
	#10;
	c = 4'b0111;
	d = 4'b0110;
        #10;
        $finish;
end

endmodule
