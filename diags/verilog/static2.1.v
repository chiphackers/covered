module main;

wire     a, b;
reg      c;

assign a = 1'b0 & c;
assign b = 1'b1 & c;

initial begin
`ifndef VPI
        $dumpfile( "static2.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
	$finish;
end

endmodule
