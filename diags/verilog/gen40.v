module main;

wire a, b, c;

foo f( .* );

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
