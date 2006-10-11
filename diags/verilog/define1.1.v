`define VALUE0    1'b0

module main;

wire	a;
reg     b;

assign a = `VALUE0 | b;

initial begin
`ifndef VPI
        $dumpfile( "define1.1.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
	#5;
	b = 1'b1;
        #20;
	$finish;
end

endmodule
