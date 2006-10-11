module main;

reg a;

initial begin
	delay_for_a;
	delay_for_a;
	delay_for_a;
	delay_for_a;
end

initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
	#5;
	a = 1'b0;
	#5;
	a = 1'b0;
	#5;
	a = 1'b1;
end

initial begin
`ifndef VPI
        $dumpfile( "task2.3.vcd" );
        $dumpvars( 0, main );
`endif
	#100;
	$finish;
end

task delay_for_a;

begin
  @(posedge a);
end

endtask

endmodule
