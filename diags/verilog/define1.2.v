	`ifdef RUNTEST
module main;

wire	a;
reg     b;

	`define VALUE0    1'b0

assign a = `VALUE0 | b;

initial begin
        $dumpfile( "define1.2.vcd" );
        $dumpvars( 0, main );
	b = 1'b0;
	#5;
	b = 1'b1;
        #20;
	$finish;
end

endmodule
	`endif
