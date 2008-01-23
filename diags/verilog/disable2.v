module main;

reg a;

initial begin
	fork
	  foo;
          begin
           #20;
	   disable foo;
	   a = 1'b1;
          end
        join
end

initial begin
`ifdef DUMP
        $dumpfile( "disable2.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

task foo;
  reg a;
  begin : bar
    a = 1'b0;
    #15;
    a = 1'b1;
    #15;
    a = 1'b0;
  end
endtask

endmodule
