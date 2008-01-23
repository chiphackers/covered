module main;

wire     a, b, c, d;

assign a = 1'b0 & 1'b0;
assign b = 1'b0 & 1'b1;
assign c = 1'b1 & 1'b0;
assign d = 1'b1 & 1'b1;

initial begin
`ifdef DUMP
        $dumpfile( "static2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
	$finish;
end

endmodule
