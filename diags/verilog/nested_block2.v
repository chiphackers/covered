module main;

wire a = foo( b );
reg  b;

initial begin
        $dumpfile( "nested_block2.vcd" );
        $dumpvars( 0, main );
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
