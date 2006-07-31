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
	y = foo_func( 1'b0 );
end

initial begin
        $dumpfile( "generate9.2.vcd" );
        $dumpvars( 0, main );
        #20;
        $finish;
end

endmodule
