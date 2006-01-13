module main;

initial begin
        $dumpfile( "task5.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

task foo;
  begin
    $display( "Hello" );
  end
endtask

task foo;
  begin
    $display( "World" );
  end
endtask

endmodule
