module main;

reg a, b;

initial begin
	fork
	  wait_for_a;
          wait_for_b;
        join
end

initial begin
`ifdef DUMP
        $dumpfile( "fork1.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1'b0;
	b = 1'b0;
 	#10;
	a = 1'b1;
	#10;
	b = 1'b1;
        #100;
        $finish;
end

task wait_for_a;
  reg c;
  begin
    c = 1'b0;
    @(posedge a);
    c = 1'b1;
  end
endtask

task wait_for_b;
  reg c;
  begin
    c = 1'b0;
    @(posedge b);
    c = 1'b1;
  end
endtask

endmodule
