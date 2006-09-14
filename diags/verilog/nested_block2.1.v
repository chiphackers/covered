module main;

reg  b;
wire a = foo( b );

initial begin
        $dumpfile( "nested_block2.1.vcd" );
        $dumpvars( 0, main );
	b = 1'b0;
        #10;
        $finish;
end

function foo;
  input a;
  reg c;
  begin
    begin : foo_block
      c = 1'b1;
    end
    foo = c;
  end
endfunction

endmodule
