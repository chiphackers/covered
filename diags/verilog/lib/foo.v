module foo( a );

input     a;

bar b( a );

endmodule

module dude( a );

input     a;
wire      a, b;

assign b = a & 1'b0;

endmodule

module bar( a );

input a;

dude d( a );

endmodule
