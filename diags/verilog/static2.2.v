module main;

parameter VAL0 = 1'b0;
parameter VAL1 = 1'b1;

wire     a, b, c, d;

assign a = 1'b0 & VAL0;
assign b = 1'b0 & VAL1;
assign c = 1'b1 & VAL0;
assign d = 1'b1 & VAL1;

initial begin
        $dumpfile( "static2.2.vcd" );
        $dumpvars( 0, main );
        #10;
	$finish;
end

endmodule
