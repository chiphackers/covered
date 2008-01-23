module main;

reg a;

initial begin
	foo;
end

initial begin
`ifdef DUMP
        $dumpfile( "nested_block3.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

task foo;
  reg a;
  begin : foo_block
    reg b;
    b = 1'b0;
    a = 1'b1;
    bar;
  end
endtask

task bar;
  begin : bar_block
    a = 1'b0;
    #10;
    a = 1'b1;
  end
endtask

endmodule
