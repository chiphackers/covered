module mux( clock, reset, a, b, sel, z );

input		clock;
input		reset;
input		a;
input		b;
input		sel;
output		z;

reg		z;

always @(posedge clock)
  if( reset )
    z = 1'b0;
  else
    if( sel )
      z = a;
    else
      z = b;

endmodule
