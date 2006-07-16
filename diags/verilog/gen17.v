module main;

foo #(.bar(2)) f();

endmodule

//-------------------------

module foo;

parameter bar = 4;

endmodule
