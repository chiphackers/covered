module main;

initial begin
	foo;
end

initial begin
`ifdef DUMP
        $dumpfile( "disable2.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

task foo;
  reg a;
  begin
    a = 1'b1;
    if( a )
      disable foo;
    else
      a = 1'b0;
  end
endtask

endmodule
