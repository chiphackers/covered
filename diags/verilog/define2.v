module main;

`define VALUE0    1'b0
`define VALUE1    1'b1
`define VALUE_AND (`VALUE0 & `VALUE1)
`define VALUE_OR  (`VALUE0 | `VALUE1)

wire     a, b;
reg      sel;

assign a = sel ? `VALUE0 : `VALUE1;
assign b = sel ? `VALUE_AND : `VALUE_OR;

initial begin
`ifdef DUMP
	$dumpfile( "define2.vcd" );
	$dumpvars( 0, main );
`endif
	sel = 1'b0;
	#10;
	sel = 1'b1;
	#10;
	$finish;
end

endmodule
