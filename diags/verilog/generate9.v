module main;

localparam A = 0;

generate
  if( A < 1 )
    task foo_task;
      reg x;
      begin
        x = 1'b0;
        #10;
        x = 1'b1;
      end
    endtask
  else
    task foo_task;
      reg x;
      begin
        x = 1'b1;
        #10;
        x = 1'b0;
      end
    endtask
endgenerate

initial begin
	foo_task;
end

initial begin
`ifdef DUMP
        $dumpfile( "generate9.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
