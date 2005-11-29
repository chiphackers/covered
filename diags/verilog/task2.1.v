module main;

reg a, c;

initial begin
	delay_for_awhile;
	a = 1'b0;
	#10;
end

initial begin
	c = 1'b0;
	#10;
	c = 1'b1;
end

initial begin
        $dumpfile( "task2.1.vcd" );
        $dumpvars( 0, main );
	#100;
	$finish;
end

task delay_for_awhile;

reg b;

begin
  b = 1'b0;
  @(posedge c);
  b = 1'b1;
end

endtask

endmodule
