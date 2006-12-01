module foo_module( foo_c );

input foo_c;

wire foo_a;
reg  foo_b;
reg  foo_d;

assign foo_a = foo_func( 1'b0 );

initial begin
	foo_task;
end

initial begin : foo_named_block
	foo_d = 1'b1;
end

function foo_func;
  input a;
  begin
    foo_func = ~a;
  end
endfunction

task foo_task;
  begin
    foo_b = 1'b0;
    #5;
    foo_b = 1'b1;
  end
endtask

endmodule
