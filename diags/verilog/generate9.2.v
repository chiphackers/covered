module main;

localparam A = 0;

reg y;

generate
  if( A < 1 )
    function foo_func;
      input a;
      begin
        foo_func = a;
      end
    endfunction
  else
    function foo_func;
      input a;
      begin
        foo_func = ~a;
      end
    endfunction
endgenerate

initial begin
	y = 1'b0;
	y = foo_func( 1'b0 );
end

initial begin
`ifdef DUMP
        $dumpfile( "generate9.2.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
