/*
 Verifies that a parameter override based on the value of a parameter
 override works correctly in sizing the signal.
 */
module main;

foo #(1) a();
goo      b();

initial begin
`ifdef DUMP
	$dumpfile( "param7.vcd" );
	$dumpvars( 0, main );
`endif
	#50;
	$finish;
end

endmodule

/******************************************/

module foo();

parameter temp = 0;

bar #(temp + 1) c();

endmodule

/******************************************/

module bar();

parameter temp = 0;

wire [(temp-1):0] a = 0;

endmodule

/******************************************/

module goo();

foo #(2) d();

endmodule
