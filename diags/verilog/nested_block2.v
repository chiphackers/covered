module main;

reg  b;
wire a = foo( b );

initial begin
`ifdef DUMP
        $dumpfile( "nested_block2.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
        #10;
        $finish;
end

function foo;
  input a;
  begin : foo_block
    reg c;
    c   = 1'b1;
    foo = bar( c );
  end
endfunction

function bar;
  input a;
  begin
    bar = ~a;
  end
endfunction

endmodule
