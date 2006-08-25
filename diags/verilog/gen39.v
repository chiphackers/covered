module main;

wire a, b, c;

foo f( .a, .b, .c );

endmodule

//-------------------------------------------

module foo(
  a,
  b,
  c
);

input a;
input b;
input c;

endmodule
