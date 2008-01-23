module main;

reg a;

initial begin
	delay_for_awhile;
	a = 1'b0;
	#10;
end

initial begin
`ifdef DUMP
        $dumpfile( "task2.vcd" );
        $dumpvars( 0, main );
`endif
	#100;
	$finish;
end

task delay_for_awhile;

reg b;

begin
  b = 1'b0;
  #10;
  b = 1'b1;
end

endtask

endmodule
