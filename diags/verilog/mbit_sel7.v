module main;

wire [31:0] a;
reg  [1:0]  b;

assign a[2-1:0] = b;

initial begin
`ifdef DUMP
        $dumpfile( "mbit_sel7.vcd" );
        $dumpvars( 0, main );
`endif
	b = 2'b00;
	#10;
	b = 2'b01;
	#10;
	b = 2'b10;
        #10;
        $finish;
end

endmodule
