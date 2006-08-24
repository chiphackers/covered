module main;

wire a, b, c;

foo f( .a, .b, .c );

endmodule

//-------------------------------------------

module foo(
  input wire a,
  input wire b,
  input wire c
);

endmodule
